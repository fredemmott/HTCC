// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <shellapi.h>
#include <wil/com.h>
#include <wil/registry.h>
#include <dwmapi.h>
#include <wil/resource.h>
#include <winuser.h>

#include <filesystem>
#include <format>
#include <optional>

#include "../lib/Config.h"
#include "../lib/PointCtrlSource.h"
#include "version.h"

#include <shlobj_core.h>

namespace HTCC = HandTrackedCockpitClicking;
namespace Config = HTCC::Config;
namespace Version = HTCCSettings::Version;

static const auto VersionString = std::format(
  "HTCC {}\n\n"
  "Copyright © 2022 Frederick Emmott.\n\n"
  "Build: v{}.{}.{}.{}-{}-{}-{}",
  Version::ReleaseName,
  Version::Major,
  Version::Minor,
  Version::Patch,
  Version::Build,
  Version::IsGitHubActionsBuild ? std::format("GHA-{}", Version::Build)
                                : "local",
  Version::BuildMode,
#ifdef _WIN32
#ifdef _WIN64
  "Win64"
#else
  "Win32"
#endif
#endif
);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND hWnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam);

template<const GUID& TFolderID> auto GetKnownFolderPath() {
    static std::filesystem::path sPath;
    static std::once_flag sOnce;
    std::call_once(sOnce, [&path = sPath]() {
      wil::unique_cotaskmem_string buf;
      THROW_IF_FAILED(SHGetKnownFolderPath(TFolderID, KF_FLAG_DEFAULT, nullptr, buf.put()));
      path = { std::wstring_view { buf.get() } };
      if (std::filesystem::exists(path)) {
        path = std::filesystem::canonical(path);
      }
    });
  return sPath;
}

class HTCCSettingsApp {
 public:
  HTCCSettingsApp() = delete;
  explicit HTCCSettingsApp(const HINSTANCE instance) {
    gInstance = this;
    Config::LoadBaseConfig();

    const WNDCLASSW wc {
      .lpfnWndProc = &WindowProc,
      .hInstance = instance,
      .lpszClassName = L"HTCC Settings",
    };
    const auto classAtom = RegisterClassW(&wc);

    {
      const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
      const auto height = screenHeight / 2;
      const auto width = (height * 2) / 3;

      mHwnd.reset(CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_CLIENTEDGE,
        MAKEINTATOM(classAtom),
        L"HTCC Settings",
        WS_OVERLAPPEDWINDOW & (~WS_MAXIMIZEBOX),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        instance,
        nullptr));
    }
    if (!mHwnd) {
      throw std::runtime_error(
        std::format("Failed to create window: {}", GetLastError()));
    }
    {
      BOOL darkMode { true};
      // Support building with the Windows 10 SDK
      #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
      #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
      #endif
      DwmSetWindowAttribute(mHwnd.get(), DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
    RECT clientRect {};
    GetClientRect(mHwnd.get(), &clientRect);

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
      .Width = static_cast<UINT>(clientRect.right - clientRect.left),
      .Height = static_cast<UINT>(clientRect.bottom - clientRect.top),
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = {1, 0},
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
      .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
    };
    mWindowSize = {
      static_cast<FLOAT>(swapChainDesc.Width),
      static_cast<FLOAT>(swapChainDesc.Height),
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
    io.IniFilename = mIniPath.c_str();

    ImGui_ImplWin32_Init(mHwnd.get());
    ImGui_ImplDX11_Init(mD3DDevice.get(), mD3DContext.get());

    this->LoadFonts();
  }

  ~HTCCSettingsApp() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    gInstance = nullptr;
  }

  void LoadFonts() {
    static const auto sFontsPath = GetKnownFolderPath<FOLDERID_Fonts>();
    if (sFontsPath.empty()) {
      return;
    }

    auto fonts = ImGui::GetIO().Fonts;
    fonts->Clear();

    const auto dpi = GetDpiForWindow(mHwnd.get());
    const auto scale = static_cast<FLOAT>(dpi) / USER_DEFAULT_SCREEN_DPI;
    const auto size = std::floorf(scale * 16);

    fonts->AddFontFromFileTTF(
      (sFontsPath / "segoeui.ttf").string().c_str(), size);
    fonts->Build();
    ImGui_ImplDX11_InvalidateDeviceObjects();

    ImGui::GetStyle().ScaleAllSizes(scale);
  }

  [[nodiscard]] HWND GetHWND() const noexcept {
    return mHwnd.get();
  }

  [[nodiscard]] int Run() noexcept {
    while (!mExitCode) {
      if (mPendingResize) {
        mRenderTargetView.reset();
        const auto width = std::get<0>(*mPendingResize);
        const auto height = std::get<1>(*mPendingResize);
        mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        wil::com_ptr<ID3D11Texture2D> backBuffer;
        THROW_IF_FAILED(
          mSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put())));
        THROW_IF_FAILED(mD3DDevice->CreateRenderTargetView(
          backBuffer.get(), nullptr, mRenderTargetView.put()));

        mWindowSize = {
          static_cast<FLOAT>(width),
          static_cast<FLOAT>(height),
        };
        mPendingResize = std::nullopt;
      }

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
  static HTCCSettingsApp* gInstance;
  wil::unique_hwnd mHwnd;
  wil::com_ptr<IDXGISwapChain1> mSwapChain;
  wil::com_ptr<ID3D11Device> mD3DDevice;
  wil::com_ptr<ID3D11DeviceContext> mD3DContext;
  wil::com_ptr<ID3D11RenderTargetView> mRenderTargetView;
  std::optional<int> mExitCode;
  ImVec2 mWindowSize;
  std::optional<std::tuple<UINT, UINT>> mPendingResize;

  static LRESULT
  WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
      return true;
    }
    if (uMsg == WM_SIZE) {
      const UINT width = LOWORD(lParam);
      const UINT height = HIWORD(lParam);
      gInstance->mPendingResize = std::tuple {width, height};
      return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

  bool mIsAPILayerEnabled = IsAPILayerEnabled();

  static std::wstring GetAPILayerPath() {
    wchar_t buf[MAX_PATH];
    const auto bufLen = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::weakly_canonical(
             std::filesystem::path(std::wstring_view {buf, bufLen})
               .parent_path()
               .parent_path()
             / "APILayer.json")
      .wstring();
  }

  static constexpr wchar_t APILayerSubkey[]
    = L"SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\Implicit";

  static bool IsAPILayerEnabled() noexcept {
    const auto path = GetAPILayerPath();
    const auto disabled = wil::reg::try_get_value_dword(
                            HKEY_LOCAL_MACHINE, APILayerSubkey, path.c_str())
                            .value_or(1);
    return !disabled;
  }

  HTCC::PointCtrlSource mPointCtrl;

  void FrameTick() noexcept {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(mWindowSize);
    ImGui::Begin(
      "MainWindow",
      nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::SeparatorText("Common Settings");
    if (ImGui::Checkbox("Enable HTCC", &mIsAPILayerEnabled)) {
      const auto apiLayerPath = GetAPILayerPath();
      const DWORD disabled = mIsAPILayerEnabled ? 0 : 1;
      wil::reg::set_value_dword(HKEY_LOCAL_MACHINE, APILayerSubkey, disabled);
    }
    if (ImGui::Checkbox(
          "Enable hibernation gesture",
          &Config::HandTrackingHibernateGestureEnabled)) {
      Config::SaveHandTrackingHibernateGestureEnabled();
    }
    Gui_PointerSource();
    Gui_PointerSink();

    ImGui::SeparatorText("Gestures");
    ImGui::TextWrapped(
      "Gestures require the XR_FB_hand_tracking_aim extension; for Meta Link, "
      "this requires developer mode.");
    if (ImGui::Checkbox("Enable pinch to click", &Config::PinchToClick)) {
      Config::SavePinchToClick();
    }
    if (ImGui::Checkbox("Enable pinch to scroll", &Config::PinchToScroll)) {
      Config::SavePinchToScroll();
    }

    ImGui::SeparatorText("PointCTRL");
    Gui_CalibratePointCtrl();
    Gui_PointCtrlFCUMapping();

    ImGui::SeparatorText("Workarounds");
    {
      auto ignoreAimPose = !Config::UseHandTrackingAimPointFB;
      if (ImGui::Checkbox(
            "Ignore XR_FB_hand_tracking_aim pose", &ignoreAimPose)) {
        Config::SaveUseHandTrackingAimPointFB(!ignoreAimPose);
      }
    }
    ImGui::TextWrapped(
      "HTCC attempts to detect available features; this may not work with some "
      "buggy drivers. You can bypass the detection below - if the features are "
      "not actually available, this may make games crash.");
    if (ImGui::Checkbox(
          "Always enable XR_ext_hand_tracking",
          &Config::ForceHaveXRExtHandTracking)) {
      Config::SaveForceHaveXRExtHandTracking();
    }
    if (ImGui::Checkbox(
          "Always enable XR_FB_hand_tracking_aim",
          &Config::ForceHaveXRFBHandTrackingAim)) {
      Config::SaveForceHaveXRFBHandTrackingAim();
    }

    ImGui::SeparatorText("About HTCC");
    ImGui::TextWrapped("%s", VersionString.c_str());
    if (ImGui::Button("Copy")) {
      ImGui::SetClipboardText(VersionString.c_str());
    }

    ImGui::End();
  }

  void Gui_CalibratePointCtrl() {
    ImGui::BeginDisabled(!mPointCtrl.IsConnected());
    const bool clicked = ImGui::Button("Calibrate");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("Requires PointCTRL device with HTCC firmware");
    if (!clicked) {
      return;
    }

    wchar_t myPath[MAX_PATH];
    const auto myPathLen = GetModuleFileNameW(nullptr, myPath, MAX_PATH);
    const auto calibrationExe
      = std::filesystem::weakly_canonical(
          std::filesystem::path(std::wstring_view {myPath, myPathLen})
            .parent_path()
            .parent_path()
          / L"PointCtrlCalibration.exe")
          .wstring();
    ShellExecuteW(
      mHwnd.get(),
      L"open",
      calibrationExe.c_str(),
      calibrationExe.c_str(),
      nullptr,
      SW_NORMAL);
  }

  void Gui_PointerSource() {
    constexpr const char* labels[] = {
      "OpenXR hand tracking",
      "PointCTRL",
    };
    auto index = static_cast<int>(Config::PointerSource);
    if (!ImGui::BeginCombo("Hand tracking method", labels[index], 0)) {
      return;
    }

    for (int i = 0; i < std::size(labels); ++i) {
      const auto isSelected = (index == i);
      if (ImGui::Selectable(labels[i], isSelected)) {
        index = i;
        Config::SavePointerSource(static_cast<HTCC::PointerSource>(i));
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  void Gui_PointerSink() {
    constexpr const char* labels[] = {
      "Emulated touch screen or tablet",
      "Emulated Oculus Touch controller",
    };
    auto index = static_cast<int>(Config::PointerSink);
    if (!ImGui::BeginCombo("Preferred game input", labels[index], 0)) {
      return;
    }

    for (int i = 0; i < std::size(labels); ++i) {
      const auto isSelected = (index == i);
      if (ImGui::Selectable(labels[i], isSelected)) {
        index = i;
        Config::SavePointerSink(static_cast<HTCC::PointerSink>(i));
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  void Gui_PointCtrlFCUMapping() {
    constexpr const char* labels[] = {
      "Disabled",
      "Classic",
      "Modal",
      "Modal with left click lock",
      "Dedicated scroll buttons",
    };
    auto index = static_cast<int>(Config::PointCtrlFCUMapping);
    if (!ImGui::BeginCombo("FCU button mapping", labels[index], 0)) {
      return;
    }

    for (int i = 0; i < std::size(labels); ++i) {
      if (
        i == static_cast<int>(HTCC::PointCtrlFCUMapping::DedicatedScrollButtons)
        && index != i) {
        continue;
      }
      const auto isSelected = (index == i);
      if (ImGui::Selectable(labels[i], isSelected)) {
        index = i;
        Config::SavePointCtrlFCUMapping(
          static_cast<HTCC::PointCtrlFCUMapping>(i));
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  const std::string mIniPath = (GetKnownFolderPath<FOLDERID_LocalAppData>() / "Fred Emmmott" / "HTCC" / "imgui.ini").string();
};

HTCCSettingsApp* HTCCSettingsApp::gInstance {nullptr};

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