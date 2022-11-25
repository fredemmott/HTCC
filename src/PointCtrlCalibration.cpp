/*
 * MIT License
 *
 * Copyright (c) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include <d2d1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <winrt/base.h>

#include <chrono>
#include <thread>

#include "PointCtrlSource.h"

#define EXTENSION_FUNCTIONS IT(xrGetD3D11GraphicsRequirementsKHR)

#define IT(x) static PFN_##x ext_##x {nullptr};
EXTENSION_FUNCTIONS
#undef IT

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

constexpr uint32_t TextureHeight = 1024;
constexpr uint32_t TextureWidth = 1024;

enum class CalibrationState {
  WaitForCenter,
  WaitForCenterRelease,
  WaitForOffset,
  WaitForOffsetRelease,
  Test,
};

static winrt::com_ptr<IDXGIAdapter1> GetDXGIAdapter(LUID luid) {
  winrt::com_ptr<IDXGIFactory1> dxgi;
  winrt::check_hresult(CreateDXGIFactory1(IID_PPV_ARGS(dxgi.put())));

  UINT i = 0;
  winrt::com_ptr<IDXGIAdapter1> it;
  while (dxgi->EnumAdapters1(i++, it.put()) == S_OK) {
    DXGI_ADAPTER_DESC1 desc {};
    winrt::check_hresult(it->GetDesc1(&desc));
    if (memcmp(&luid, &desc.AdapterLuid, sizeof(LUID)) == 0) {
      return it;
    }
    it = {nullptr};
  }

  return {nullptr};
}

XrInstance gInstance {};

void check_xr(XrResult result) {
  if (result == XR_SUCCESS) {
    return;
  }

  std::string message;
  if (gInstance) {
    char buffer[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(gInstance, result, buffer);
    message = std::format(
      "OpenXR call failed: '{}' ({})", buffer, static_cast<int>(result));
  } else {
    message = std::format("OpenXR call failed: {}", static_cast<int>(result));
  }
  OutputDebugStringA(message.c_str());
  throw new std::runtime_error(message);
}

struct DrawingResources {
  winrt::com_ptr<ID3D11Texture2D> mTexture;
  winrt::com_ptr<ID2D1RenderTarget> mRT;
  winrt::com_ptr<ID2D1SolidColorBrush> mBrush;
  winrt::com_ptr<IDWriteTextFormat> mTextFormat;
};

static DrawingResources sDrawingResources;

void InitDrawingResources(ID3D11DeviceContext* context) {
  auto& res = sDrawingResources;
  if (res.mRT) [[likely]] {
    return;
  }

  winrt::com_ptr<ID3D11Device> device;
  context->GetDevice(device.put());
  D3D11_TEXTURE2D_DESC desc {
    .Width = TextureWidth,
    .Height = TextureHeight,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = DXGI_FORMAT_B8G8R8A8_UNORM,// needed for Direct2D
    .SampleDesc = {1, 0},
    .BindFlags = D3D11_BIND_RENDER_TARGET,
  };
  winrt::check_hresult(
    device->CreateTexture2D(&desc, nullptr, res.mTexture.put()));
  auto surface = res.mTexture.as<IDXGISurface>();

  winrt::com_ptr<ID2D1Factory> d2d;
  winrt::check_hresult(
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d.put()));

  winrt::check_hresult(d2d->CreateDxgiSurfaceRenderTarget(
    surface.get(),
    D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_HARDWARE,
      D2D1::PixelFormat(
        DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
    res.mRT.put()));
  res.mRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
  // Don't use cleartype as subpixels won't line up in headset
  res.mRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

  winrt::check_hresult(res.mRT->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Black, 1.0f), res.mBrush.put()));

  winrt::com_ptr<IDWriteFactory> dwrite;
  winrt::check_hresult(DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_ISOLATED,
    __uuidof(dwrite),
    reinterpret_cast<IUnknown**>(dwrite.put())));

  winrt::check_hresult(dwrite->CreateTextFormat(
    L"Calibri",
    nullptr,
    DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH_NORMAL,
    64.0f,
    L"",
    res.mTextFormat.put()));
}

void DrawLayer(
  CalibrationState state,
  ID3D11DeviceContext* context,
  ID3D11Texture2D* texture,
  XrPosef* pose) {
  InitDrawingResources(context);

  auto& res = sDrawingResources;
  auto rt = res.mRT.get();
  auto brush = res.mBrush.get();

  rt->BeginDraw();
  rt->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));

  // draw crosshairs
  rt->DrawLine(
    {TextureWidth / 2.0, 0}, {TextureWidth / 2.0, TextureHeight}, brush, 5.0f);
  rt->DrawLine(
    {0, TextureHeight / 2.0}, {TextureWidth, TextureHeight / 2.0}, brush, 5.0f);

  std::wstring_view message;
  *pose = {
    .orientation = XR_POSEF_IDENTITY.orientation,
    .position = {0.0f, 0.0f, -1.5f},
  };
  message = L"Reach for the center of the crosshair, then press FCU button 1";

  rt->DrawTextW(
    message.data(),
    message.size(),
    res.mTextFormat.get(),
    {5.0f, 5.0f, (TextureWidth / 2.0f) - 7.5f, (TextureHeight / 2.0f) - 7.5f},
    brush);

  winrt::check_hresult(rt->EndDraw());
  context->CopyResource(texture, res.mTexture.get());
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  XrInstance instance {};
  {
    const std::vector<const char*> enabledExtensions = {
      XR_KHR_D3D11_ENABLE_EXTENSION_NAME,
    };
    XrInstanceCreateInfo createInfo {
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .applicationInfo = {
        .applicationName = "PointCtrl Calibration",
        .applicationVersion = 1,
        .apiVersion = XR_CURRENT_API_VERSION,
      },
      .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
      .enabledExtensionNames = enabledExtensions.data(),
    };
    check_xr(xrCreateInstance(&createInfo, &instance));
    gInstance = instance;
  }

#define IT(x) \
  xrGetInstanceProcAddr( \
    instance, #x, reinterpret_cast<PFN_xrVoidFunction*>(&ext_##x));
  EXTENSION_FUNCTIONS
#undef IT

  XrSystemId system {};
  {
    XrSystemGetInfo getInfo {
      .type = XR_TYPE_SYSTEM_GET_INFO,
      .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };
    xrGetSystem(instance, &getInfo, &system);
  }

  winrt::com_ptr<ID3D11Device> device;
  winrt::com_ptr<ID3D11DeviceContext> context;
  {
    XrGraphicsRequirementsD3D11KHR d3dRequirements {
      XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    ext_xrGetD3D11GraphicsRequirementsKHR(instance, system, &d3dRequirements);

    auto adapter = GetDXGIAdapter(d3dRequirements.adapterLuid);
    D3D_FEATURE_LEVEL featureLevels[] = {d3dRequirements.minFeatureLevel};

    UINT flags {D3D11_CREATE_DEVICE_BGRA_SUPPORT};
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    winrt::check_hresult(D3D11CreateDevice(
      adapter.get(),
      D3D_DRIVER_TYPE_UNKNOWN,
      0,
      flags,
      featureLevels,
      _countof(featureLevels),
      D3D11_SDK_VERSION,
      device.put(),
      nullptr,
      context.put()));
  }

  XrSession session {};
  {
    XrGraphicsBindingD3D11KHR binding {
      .type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR,
      .device = device.get(),
    };
    XrSessionCreateInfo createInfo {
      .type = XR_TYPE_SESSION_CREATE_INFO,
      .next = &binding,
      .systemId = system,
    };
    check_xr(xrCreateSession(instance, &createInfo, &session));
  }

  XrSpace viewSpace {};
  {
    XrReferenceSpaceCreateInfo createInfo {
      .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
      .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
      .poseInReferenceSpace = XR_POSEF_IDENTITY,
    };
    xrCreateReferenceSpace(session, &createInfo, &viewSpace);
  }

  bool xrRunning = false;
  XrSwapchain swapchain {};
  std::vector<XrSwapchainImageD3D11KHR> swapchainImages;
  CalibrationState state {CalibrationState::WaitForCenter};

  while (true) {
    {
      XrEventDataBuffer event {XR_TYPE_EVENT_DATA_BUFFER};
      while (xrPollEvent(instance, &event) == XR_SUCCESS) {
        switch (event.type) {
          case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            return 0;
          case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            const auto state
              = reinterpret_cast<XrEventDataSessionStateChanged*>(&event)
                  ->state;
            switch (state) {
              case XR_SESSION_STATE_READY: {
                XrSessionBeginInfo beginInfo {
                  .type = XR_TYPE_SESSION_BEGIN_INFO,
                  .primaryViewConfigurationType
                  = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                };
                check_xr(xrBeginSession(session, &beginInfo));
                xrRunning = true;

                XrSwapchainCreateInfo createInfo {
                  .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                  .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT
                    | XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT,
                  .format = DXGI_FORMAT_B8G8R8A8_UNORM,
                  .sampleCount = 1,
                  .width = TextureWidth,
                  .height = TextureHeight,
                  .faceCount = 1,
                  .arraySize = 1,
                  .mipCount = 1,
                };
                check_xr(xrCreateSwapchain(session, &createInfo, &swapchain));
                uint32_t swapchainLength {};
                check_xr(xrEnumerateSwapchainImages(
                  swapchain, 0, &swapchainLength, nullptr));
                swapchainImages.resize(
                  swapchainLength, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
                check_xr(xrEnumerateSwapchainImages(
                  swapchain,
                  swapchainImages.size(),
                  &swapchainLength,
                  reinterpret_cast<XrSwapchainImageBaseHeader*>(
                    swapchainImages.data())));
                break;
              }
              case XR_SESSION_STATE_STOPPING:
                return 0;
              case XR_SESSION_STATE_EXITING:
                return 0;
              case XR_SESSION_STATE_LOSS_PENDING:
                return 0;
              default:
                break;
            }
          }
          default:
            break;
        }
      }
    }// event handling

    if (!xrRunning) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // FRAME STARTS HERE
    XrFrameState frameState {XR_TYPE_FRAME_STATE};
    check_xr(xrWaitFrame(session, nullptr, &frameState));
    check_xr(xrBeginFrame(session, nullptr));

    XrCompositionLayerQuad layer {
      .type = XR_TYPE_COMPOSITION_LAYER_QUAD,
      .space = viewSpace,
      .subImage = {
        .swapchain = swapchain,
        .imageRect = {{0, 0}, {TextureWidth, TextureHeight}},
        .imageArrayIndex = 0,
      },
      .size = {0.5f, 0.5f},
    };

    {
      uint32_t imageIndex;
      check_xr(xrAcquireSwapchainImage(swapchain, nullptr, &imageIndex));
      XrSwapchainImageWaitInfo waitInfo {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
        .timeout = XR_INFINITE_DURATION,
      };
      check_xr(xrWaitSwapchainImage(swapchain, &waitInfo));

      DrawLayer(
        state,
        context.get(),
        swapchainImages.at(imageIndex).texture,
        &layer.pose);
      check_xr(xrReleaseSwapchainImage(swapchain, nullptr));
    }

    XrCompositionLayerQuad* layerPtr = &layer;

    XrFrameEndInfo endInfo {
      .type = XR_TYPE_FRAME_END_INFO,
      .displayTime = frameState.predictedDisplayTime,
      .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
      .layerCount = 1,
      .layers = reinterpret_cast<XrCompositionLayerBaseHeader**>(&layerPtr),
    };

    check_xr(xrEndFrame(session, &endInfo));
  }
}
