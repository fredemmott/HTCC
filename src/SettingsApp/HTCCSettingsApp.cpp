// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <winuser.h>

#include <optional>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND hWnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam);

class HTCCSettingsApp {
 public:
  HTCCSettingsApp() = delete;
  explicit HTCCSettingsApp(const HINSTANCE instance) {
    const WNDCLASSW wc {
      .lpfnWndProc = &WindowProc,
      .hInstance = instance,
      .lpszClassName = L"HTCC Settings",
    };
    const auto classAtom = RegisterClass(&wc);

    mHwnd.reset(CreateWindowExW(
      WS_EX_APPWINDOW | WS_EX_CLIENTEDGE,
      wc.lpszClassName,
      L"HTCC Settings",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      nullptr,
      nullptr,
      instance,
      nullptr));

    RECT windowRect {};
    GetClientRect(mHwnd.get(), &windowRect);

    UINT d3dFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
    UINT dxgiFlags = 0;
    d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
    dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;

    THROW_IF_FAILED(D3D11CreateDevice(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      d3dFlags,
      nullptr,
      0,
      D3D11_SDK_VERSION,
      mD3DDevice.put(),
      nullptr,
      mD3DContext.put()));

    wil::com_ptr<IDXGIFactory3> dxgiFactory;
    THROW_IF_FAILED(
      CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(dxgiFactory.put())));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {
      .Width = static_cast<UINT>(windowRect.right - windowRect.left),
      .Height = static_cast<UINT>(windowRect.bottom - windowRect.top),
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = {1, 0},
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
      .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
    };
    THROW_IF_FAILED(dxgiFactory->CreateSwapChainForHwnd(
      mD3DDevice.get(),
      mHwnd.get(),
      &swapChainDesc,
      nullptr,
      nullptr,
      mSwapChain.put()));

    wil::com_ptr<ID3D11Texture2D> backBuffer;
    THROW_IF_FAILED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put())));
    THROW_IF_FAILED(mD3DDevice->CreateRenderTargetView(
      backBuffer.get(), nullptr, mRenderTargetView.put()));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(mHwnd.get());
    ImGui_ImplDX11_Init(mD3DDevice.get(), mD3DContext.get());
  }

  ~HTCCSettingsApp() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
  }

  [[nodiscard]] HWND GetHWND() const noexcept {
    return mHwnd.get();
  }

  [[nodiscard]] int Run() noexcept {
    while (!mExitCode) {
      MSG msg {};
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
          return mExitCode.value_or(0);
        }
      }

      const auto rawRTV = mRenderTargetView.get();
      FLOAT clearColor[4] {0.0f, 0.0f, 0.0f, 1.0f};
      mD3DContext->ClearRenderTargetView(rawRTV, clearColor);
      mD3DContext->OMSetRenderTargets(1, &rawRTV, nullptr);

      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
      this->FrameTick();
      ImGui::Render();
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
      mSwapChain->Present(1, 0);

      WaitMessage();
    }

    return *mExitCode;
  }

 private:
  wil::unique_hwnd mHwnd;
  wil::com_ptr<IDXGISwapChain1> mSwapChain;
  wil::com_ptr<ID3D11Device> mD3DDevice;
  wil::com_ptr<ID3D11DeviceContext> mD3DContext;
  wil::com_ptr<ID3D11RenderTargetView> mRenderTargetView;
  std::optional<int> mExitCode;

  static LRESULT
  WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
      return true;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

  void FrameTick() noexcept {
    ImGui::ShowDemoWindow();
  }
};

int WINAPI wWinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPWSTR lpCmdLine,
  int nCmdShow) {
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  HTCCSettingsApp app(hInstance);
  ShowWindow(app.GetHWND(), nCmdShow);
  return app.Run();
}