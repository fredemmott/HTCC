// Copyright (c) 2024-present Frederick Emmott
// SPDX-License-Identifier: MIT

#include <Windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <wil/com.h>
#include <wil/registry.h>
#include <wil/resource.h>
#include <winuser.h>

#include <FredEmmott/GUI.hpp>
#include <FredEmmott/GUI/StaticTheme/Common.hpp>
#include <filesystem>
#include <format>
#include <optional>
#include <string_view>

#include "../lib/Config.h"
#include "../lib/PointCtrlSource.h"
#include "CheckHResult.hpp"
#include "Licenses.hpp"
#include "version.h"

using namespace FredEmmott::GUI;
using namespace FredEmmott::GUI::Immediate;

namespace HTCC = HandTrackedCockpitClicking;
namespace Version = HTCCSettings::Version;

static HWND gWindowHandle {};

using namespace std::string_view_literals;

const auto VersionString = std::format(
  "HTCC {}\n"
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
    HTCC::CheckHResult(
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
  constexpr auto Options = std::array {
    "OpenXR hand tracking",
    "PointCTRL",
  };
  static auto idx = static_cast<std::size_t>(HTCC::Config::PointerSource);
  if (!ComboBox(&idx, Options).Caption("Hand tracking method")) {
    return;
  }

  HTCC::Config::SavePointerSource(static_cast<HTCC::PointerSource>(idx));
}

static bool& ShowUnsupportedSettings() {
  static bool sValue
    = (HTCC::Config::PointerSink == HTCC::PointerSink::VirtualVRController)
    || !HTCC::Config::UseHandTrackingAimPointFB;
  return sValue;
}

static void CommonSettingsGUI() {
  BeginCard();
  BeginVStackPanel();

  static bool isEnabled = IsAPILayerEnabled();
  if (ToggleSwitch(&isEnabled).Caption("Enable HTCC")) {
    const DWORD disabled = isEnabled ? 0 : 1;
    wil::reg::set_value_dword(HKEY_LOCAL_MACHINE, APILayerSubkey, disabled);
  }

  PointerSourceGUI();

  (void)ToggleSwitch(&ShowUnsupportedSettings())
    .Caption("Show unsupported settings");

  EndVStackPanel();
  EndCard();
}

static void UnsupportedSettingsGUI() {
  if (!ShowUnsupportedSettings()) {
    return;
  }

  Label("Unsupported settings").Subtitle();

  BeginCard();
  BeginVStackPanel();

  TextBlock(
    "These settings can cause a worse experience, and are not recommended - "
    "turn them off if you encounter any issues.");

  static bool useControllersInDCS
    = HTCC::Config::PointerSink == HTCC::PointerSink::VirtualVRController;
  if (ToggleSwitch(&useControllersInDCS)
        .Caption("Emulate VR controllers in DCS World")) {
    using enum HTCC::PointerSink;
    HTCC::Config::SavePointerSink(
      useControllersInDCS ? VirtualVRController : VirtualTouchScreen);
  }

  bool ignoreAimPose = !HTCC::Config::UseHandTrackingAimPointFB;
  if (ToggleSwitch(&ignoreAimPose)
        .Caption("Ignore XR_FB_hand_tracking_aim pose")) {
    HTCC::Config::SaveUseHandTrackingAimPointFB(!ignoreAimPose);
  }

  EndVStackPanel();
  EndCard();
}

static void GesturesGUI() {
  BeginEnabled(
    HTCC::Config::PointerSource == HTCC::PointerSource::OpenXRHandTracking);

  Label("OpenXR hand tracking").Subtitle();

  BeginCard();
  BeginVStackPanel();

  if (ToggleSwitch(&HTCC::Config::HandTrackingHibernateGestureEnabled)
        .Caption("Hold a hand up to suspend HTCC")) {
    HTCC::Config::SaveHandTrackingHibernateGestureEnabled();
  }

  if (ToggleSwitch(&HTCC::Config::PinchToClick).Caption("Pinch to click")) {
    HTCC::Config::SavePinchToClick();
  }

  if (ToggleSwitch(&HTCC::Config::PinchToScroll).Caption("Pinch to scroll")) {
    HTCC::Config::SavePinchToScroll();
  }

  EndVStackPanel();
  EndCard();
  EndEnabled();
}

static void PointCtrlCalibrationGUI() {
  static HTCC::PointCtrlSource sPointCtrl;

  BeginEnabled(sPointCtrl.IsConnected());

  Label("Calibration requires a PointCTRL device with HTCC firmware").Body();

  if (Button("Calibrate")) {
    std::array<wchar_t, 32768> myPath {};
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
  EndEnabled();
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
    HTCC::Config::PointCtrlFCUMapping
    != HTCC::PointCtrlFCUMapping::DedicatedScrollButtons) {
    --count;
  }

  static auto idx = static_cast<std::size_t>(HTCC::Config::PointCtrlFCUMapping);
  if (!ComboBox(&idx, std::views::take(Options, count))) {
    return;
  }

  HTCC::Config::SavePointCtrlFCUMapping(
    static_cast<HTCC::PointCtrlFCUMapping>(idx));
}

static void PointCtrlGUI() {
  BeginEnabled(HTCC::Config::PointerSource == HTCC::PointerSource::PointCtrl);
  Label("PointCTRL").Subtitle();
  BeginCard();
  BeginVStackPanel();

  PointCtrlCalibrationGUI();
  PointCtrlButtonMappingGUI();

  EndVStackPanel();
  EndCard();
  EndEnabled();
}

static void LicensesDialogContent() {
  static const HandTrackedCockpitClicking::Licenses licenses;
  struct Product {
    std::string_view mName;
    std::string_view mLicense;
  };
  static const auto products = std::array {
    Product {
      "Hand Tracked Cockpit Clicking (HTCC)", licenses.SelfAsStringView()},
    Product {"Compressed-Embed", licenses.CompressedEmbedAsStringView()},
    Product {"DirectXMath", licenses.DirectXMathAsStringView()},
    Product {"DirectXTK", licenses.DirectXTKAsStringView()},
    Product {"FredEmmott::GUI", licenses.FUIAsStringView()},
    Product {"OpenXR SDK", licenses.OpenXRAsStringView()},
    Product {"Windows Implementation Library", licenses.WILAsStringView()},
    Product {"Yoga", licenses.YogaAsStringView()},
  };

  const auto layout = BeginVStackPanel().Styled(Style().Gap(12)).Scoped();
  {
    const auto card = BeginCard().Scoped();
    TextBlock(
      "HTCC, Copyright © 2022-present Frederick Emmott\n\n"
      "This software contains third-party components which are separately "
      "licensed.\n\n"
      "Select a component below for details.");
  }

  static std::size_t selectedIndex {};
  ComboBox(&selectedIndex, products, &Product::mName)
    .Styled(Style().AlignSelf(YGAlignStretch));

  const auto card = BeginCard().Scoped().Styled(Style().Padding(0));
  const auto scroll
    = BeginVScrollView().Styled(Style().Width(800).Height(600)).Scoped();
  TextBlock(products[selectedIndex].mLicense).Styled(Style().Margin(8));
}

static void LicensesGUI() {
  static bool sShowingLicenses = false;
  if (HyperlinkButton("Show copyright notices")) {
    sShowingLicenses = true;
  }
  if (const auto dialog = BeginContentDialog(&sShowingLicenses).Scoped()) {
    ContentDialogTitle("Copyright notices");
    LicensesDialogContent();
    const auto buttons = BeginContentDialogButtons().Scoped();
    ContentDialogCloseButton("Close");
  }
}

static void AboutGUI() {
  {
    const auto header = BeginHStackPanel().Styled(Style().Gap(8)).Scoped();

    FontIcon("\ue74c", FontIconSize::Subtitle);

    Label("About HTCC").Subtitle().Styled(Style().FlexGrow(1));

    if (Button("Copy")) {
      if (OpenClipboard(gWindowHandle)) {
        const auto wide = HTCC::Utf8::ToWide(VersionString);
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
  }

  const auto card = BeginCard().Scoped().Styled(
    Style().Gap(8).FlexDirection(YGFlexDirectionColumn));
  TextBlock(VersionString).Body();
  LicensesGUI();
}

static void FrameTick() {
  using namespace FredEmmott::GUI::StaticTheme::Common;
  static const auto ScrollViewStyle
    = Style().BackgroundColor(LayerOnAcrylicFillColorDefaultBrush);
  BeginVScrollView().Styled(ScrollViewStyle);

  static const auto LayoutStyle = Style().Gap(12).Margin(12).Padding(8);
  BeginVStackPanel().Styled(LayoutStyle);

  CommonSettingsGUI();
  UnsupportedSettingsGUI();
  GesturesGUI();
  PointCtrlGUI();
  AboutGUI();

  EndVStackPanel();
  EndVScrollView();
}

int WINAPI wWinMain(
  HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] LPWSTR lpCmdLine,
  const int nCmdShow) {
  return Win32Window::WinMain(
    hInstance,
    hPrevInstance,
    lpCmdLine,
    nCmdShow,
    [](Win32Window&) { FrameTick(); },
    {"HTCC Settings"},
    {.mHooks = {
       .mBeforeMainLoop =
         [](Win32Window& window) {
           HTCC::Config::LoadBaseConfig();
           gWindowHandle = window.GetNativeHandle();
         },
     }});
}