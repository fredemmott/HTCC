// Copyright 2024 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <wil/com.h>
#include <wil/registry.h>
#include <wil/resource.h>
#include <winrt/base.h>
#include <winuser.h>

#include <FredEmmott/GUI.hpp>
#include <filesystem>
#include <format>
#include <optional>

#include "../lib/Config.h"
#include "../lib/PointCtrlSource.h"
#include "version.h"

namespace HTCC = HandTrackedCockpitClicking;
namespace Config = HTCC::Config;
namespace Version = HTCCSettings::Version;
namespace fui = FredEmmott::GUI;
namespace fuii = fui::Immediate;

static HWND gWindowHandle {};

const auto VersionString = std::format(
  "HTCC {}\n\n"
  "Copyright © 2022-present Frederick Emmott.\n\n"
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

template <const GUID& TFolderID>
auto GetKnownFolderPath() {
  static std::filesystem::path sPath;
  static std::once_flag sOnce;
  std::call_once(sOnce, [&path = sPath]() {
    wil::unique_cotaskmem_string buf;
    THROW_IF_FAILED(
      SHGetKnownFolderPath(TFolderID, KF_FLAG_DEFAULT, nullptr, buf.put()));
    path = {std::wstring_view {buf.get()}};
    if (std::filesystem::exists(path)) {
      path = std::filesystem::canonical(path);
    }
  });
  return sPath;
}

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

void PointerSourceGUI() {
  fuii::Caption("Hand tracking method");

  constexpr auto Options = std::array {
    "OpenXR hand tracking",
    "PointCTRL",
  };
  static auto idx = static_cast<std::size_t>(Config::PointerSource);
  if (!fuii::ComboBox(&idx, Options)) {
    return;
  }

  Config::SavePointerSource(static_cast<HTCC::PointerSource>(idx));
}

static bool& ShowUnsupportedSettings() {
  static bool sValue
    = (Config::PointerSink == HTCC::PointerSink::VirtualVRController)
    || !Config::UseHandTrackingAimPointFB;
  return sValue;
}

static void CommonSettingsGUI() {
  fuii::BeginCard();
  fuii::BeginVStackPanel();

  fuii::Caption("Enable HTCC");
  static bool isEnabled = IsAPILayerEnabled();
  if (fuii::ToggleSwitch(&isEnabled)) {
    const DWORD disabled = isEnabled ? 0 : 1;
    wil::reg::set_value_dword(HKEY_LOCAL_MACHINE, APILayerSubkey, disabled);
  }

  PointerSourceGUI();

  fuii::Caption("Show unsupported settings");
  (void)fuii::ToggleSwitch(&ShowUnsupportedSettings());

  fuii::EndVStackPanel();
  fuii::EndCard();
}

static void UnsupportedSettingsGUI() {
  if (!ShowUnsupportedSettings()) {
    return;
  }

  fuii::SubtitleLabel("Unsupported settings");

  fuii::BeginCard();
  fuii::BeginVStackPanel();

  fuii::TextBlock(
    "These settings can cause a worse experience, and are not recommended - "
    "turn them off if you encounter any issues.");

  fuii::Caption("Emulate VR controllers in DCS World");
  static bool useControllersInDCS
    = Config::PointerSink == HTCC::PointerSink::VirtualVRController;
  if (fuii::ToggleSwitch(&useControllersInDCS)) {
    using enum HTCC::PointerSink;
    Config::SavePointerSink(
      useControllersInDCS ? VirtualVRController : VirtualTouchScreen);
  }

  fuii::Caption("Ignore XR_FB_hand_tracking_aim pose");
  bool ignoreAimPose = !Config::UseHandTrackingAimPointFB;
  if (fuii::ToggleSwitch(&ignoreAimPose)) {
    Config::SaveUseHandTrackingAimPointFB(!ignoreAimPose);
  }

  fuii::EndVStackPanel();
  fuii::EndCard();
}

static void GesturesGUI() {
  fuii::BeginEnabled(
    Config::PointerSource == HTCC::PointerSource::OpenXRHandTracking);
  fuii::SubtitleLabel("OpenXR hand tracking");

  fuii::BeginCard();
  fuii::BeginVStackPanel();

  fuii::Caption("Hold a hand up to suspend HTCC");
  if (fuii::ToggleSwitch(&Config::HandTrackingHibernateGestureEnabled)) {
    Config::SaveHandTrackingHibernateGestureEnabled();
  }

  fuii::Caption("Pinch to click");
  if (fuii::ToggleSwitch(&Config::PinchToClick)) {
    Config::SavePinchToClick();
  }

  fuii::Caption("Pinch to scroll");
  if (fuii::ToggleSwitch(&Config::PinchToScroll)) {
    Config::SavePinchToScroll();
  }

  fuii::EndVStackPanel();
  fuii::EndCard();
  fuii::EndEnabled();
}

static void PointCtrlCalibrationGUI() {
  static HTCC::PointCtrlSource sPointCtrl;

  fuii::BeginEnabled(sPointCtrl.IsConnected());
  fuii::BodyLabel("Calibration requires a PointCTRL device with HTCC firmware");
  if (fuii::Button("Calibrate")) {
    std::array<wchar_t, 32768> myPath;
    const auto myPathLen
      = GetModuleFileNameW(nullptr, myPath.data(), myPath.size());
    ;
    const auto calibrationExe
      = std::filesystem::weakly_canonical(
          std::filesystem::path(std::wstring_view {myPath.data(), myPathLen})
            .parent_path()
            .parent_path()
          / L"PointCtrlCalibration.exe")
          .wstring();
    ShellExecuteW(
      nullptr,
      L"open",
      calibrationExe.c_str(),
      calibrationExe.c_str(),
      nullptr,
      SW_NORMAL);
  }
  fuii::EndEnabled();
}

static void PointCtrlButtonMappingGUI() {
  constexpr auto Options = std::array {
    "Disabled",
    "Classic",
    "Modal",
    "Modal with left click lock",
    "Dedicated scroll buttons",
  };
  auto count = Options.size();
  // Remove the deprecated option, unless it's selected
  if (
    Config::PointCtrlFCUMapping
    != HTCC::PointCtrlFCUMapping::DedicatedScrollButtons) {
    --count;
  }

  static auto idx = static_cast<std::size_t>(Config::PointCtrlFCUMapping);
  if (!fuii::ComboBox(&idx, std::views::take(Options, count))) {
    return;
  }

  Config::SavePointCtrlFCUMapping(static_cast<HTCC::PointCtrlFCUMapping>(idx));
}

static void PointCtrlGUI() {
  fuii::BeginEnabled(Config::PointerSource == HTCC::PointerSource::PointCtrl);
  fuii::SubtitleLabel("PointCTRL");
  fuii::BeginCard();
  fuii::BeginVStackPanel();

  PointCtrlCalibrationGUI();
  PointCtrlButtonMappingGUI();

  fuii::EndVStackPanel();
  fuii::EndCard();
  fuii::EndEnabled();
}

static void AboutGUI() {
  fuii::BeginHStackPanel();
  fuii::Style({.mGap = 8});

  fuii::FontIcon("\ue74c", fuii::FontIconSize::Subtitle);// OEM
  fuii::Style({.mTranslateY = -4});

  fuii::SubtitleLabel("About HTCC");
  fuii::Style({.mFlexGrow = 1});

  if (fuii::Button("Copy")) {
    if (OpenClipboard(gWindowHandle)) {
      const auto wide = winrt::to_hstring(VersionString);
      wil::unique_hglobal buf(
        GlobalAlloc(0, (wide.size() + 1) * sizeof(wchar_t)));
      memcpy(buf.get(), wide.data(), (wide.size() + 1) * sizeof(wchar_t));
      EmptyClipboard();
      if (SetClipboardData(CF_UNICODETEXT, buf.get())) {
        buf.release();// owned by windows
      }
      CloseClipboard();
    }
  }
  fuii::EndHStackPanel();

  fuii::BeginCard();
  fuii::TextBlock(VersionString);
  fuii::EndCard();
}

static void FrameTick() {
  fuii::BeginVScrollView();
  fuii::BeginVStackPanel();
  fuii::Style({
    .mGap = 12,
    .mMargin = 12,
    .mPadding = 8,
  });

  CommonSettingsGUI();
  UnsupportedSettingsGUI();
  GesturesGUI();
  PointCtrlGUI();
  AboutGUI();

  fuii::EndVStackPanel();
  fuii::EndVScrollView();
}

int WINAPI wWinMain(
  HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] LPWSTR lpCmdLine,
  const int nCmdShow) {

  return fui::Win32Window::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
    [](fui::Window&) { FrameTick(); },
    {"HTCC Settings"},
    {
      .mHooks = {
        .mBeforeMainLoop = [](fui::Window& window) {
          Config::LoadBaseConfig();
          gWindowHandle = window.GetNativeHandle();
        },
      }
    }
    );
}