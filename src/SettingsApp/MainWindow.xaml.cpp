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
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <microsoft.ui.xaml.window.h>
#include <shellapi.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.System.h>

#include <filesystem>
#include <format>

#include "../lib/Config.h"
#include "../lib/DebugPrint.h"
#include "version.h"

namespace HTCC = HandTrackedCockpitClicking;
namespace Version = HTCCSettings::Version;

static constexpr wchar_t gAPILayerSubkey[]
  = L"SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\Implicit";

namespace winrt::HTCCSettings::implementation {
MainWindow::MainWindow() {
  HTCC::Config::LoadBaseConfig();

  InitializeComponent();
  ExtendsContentIntoTitleBar(true);
  SetTitleBar(AppTitleBar());

  Title(L"HTCC Settings");
  InitVersion();

  // Get an AppWindow so that we can resize...
  auto windowNative = this->try_as<::IWindowNative>();
  if (!windowNative) {
    return;
  }
  check_hresult(windowNative->get_WindowHandle(&mHwnd));
  const auto windowId = Microsoft::UI::GetWindowIdFromWindow(mHwnd);
  const auto appWindow
    = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
  const auto size = appWindow.Size();
  // Default aspect ratio isn't appropriate for the content here
  if (size.Width > size.Height) {
    appWindow.Resize({size.Width / 3, size.Height});
  }

  // WinUI3 leaves the 'Busy' cursor up even once everything's loaded
  RootGrid().Loaded(
    [](auto, auto) { SetCursor(LoadCursorW(nullptr, IDC_ARROW)); });
}

void MainWindow::InitVersion() {
  mVersionString = std::format(
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
  VersionText().Text(to_hstring(mVersionString));
}

void MainWindow::OnCopyVersionDataClick(
  const IInspectable&,
  const MUX::RoutedEventArgs&) {
  Windows::ApplicationModel::DataTransfer::DataPackage package;
  package.SetText(winrt::to_hstring(mVersionString));
  Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
}

winrt::fire_and_forget MainWindow::OnSponsorClick(
  const IInspectable&,
  const MUX::RoutedEventArgs&) {
  co_await winrt::Windows::System::Launcher::LaunchUriAsync(
    winrt::Windows::Foundation::Uri {
      to_hstring(L"https://github.com/sponsors/fredemmott")});
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

bool MainWindow::IsEnabled() const noexcept {
  const auto apiLayerPath = GetAPILayerPath();
  HTCC::DebugPrint(L"Looking for {}", apiLayerPath);

  DWORD disabled {};
  DWORD dataSize = sizeof(disabled);
  if (
    RegGetValueW(
      HKEY_LOCAL_MACHINE,
      gAPILayerSubkey,
      apiLayerPath.c_str(),
      RRF_RT_DWORD,
      nullptr,
      &disabled,
      &dataSize)
    != ERROR_SUCCESS) {
    return false;
  }
  return disabled == 0;
}

void MainWindow::IsEnabled(bool enabled) noexcept {
  DWORD disabled(enabled ? 0 : 1);
  const auto result = RegSetKeyValueW(
    HKEY_LOCAL_MACHINE,
    gAPILayerSubkey,
    GetAPILayerPath().c_str(),
    REG_DWORD,
    &disabled,
    sizeof(disabled));
  if (result != ERROR_SUCCESS) {
    auto message = std::format("Saving to registry failed: error {}", result);
    throw std::runtime_error(message);
  }
}

int16_t MainWindow::PointerSource() const noexcept {
  return static_cast<int16_t>(HTCC::Config::PointerSource);
}

void MainWindow::PointerSource(int16_t value) noexcept {
  HTCC::Config::SavePointerSource(static_cast<HTCC::PointerSource>(value));
}

int16_t MainWindow::PointerSink() const noexcept {
  return static_cast<int16_t>(HTCC::Config::PointerSink);
}

void MainWindow::PointerSink(int16_t value) noexcept {
  HTCC::Config::SavePointerSink(static_cast<HTCC::PointerSink>(value));
}

bool MainWindow::PinchToClick() const noexcept {
  return HTCC::Config::PinchToClick;
}

void MainWindow::PinchToClick(bool value) noexcept {
  HTCC::Config::SavePinchToClick(value);
}

bool MainWindow::PinchToScroll() const noexcept {
  return HTCC::Config::PinchToScroll;
}

void MainWindow::PinchToScroll(bool value) noexcept {
  HTCC::Config::SavePinchToScroll(value);
}

int16_t MainWindow::PointCtrlFCUMapping() const noexcept {
  return static_cast<int16_t>(HTCC::Config::PointCtrlFCUMapping);
}

void MainWindow::OnPointCtrlCalibrateClick(
  const IInspectable&,
  const MUX::RoutedEventArgs&) {
  wchar_t myPath[MAX_PATH];
  const auto myPathLen = GetModuleFileNameW(NULL, myPath, MAX_PATH);
  const auto calibrationExe
    = std::filesystem::weakly_canonical(
        std::filesystem::path(std::wstring_view {myPath, myPathLen})
          .parent_path()
          .parent_path()
        / L"PointCtrlCalibration.exe")
        .wstring();
  ShellExecuteW(
    mHwnd,
    L"open",
    calibrationExe.c_str(),
    calibrationExe.c_str(),
    nullptr,
    SW_NORMAL);
}

void MainWindow::PointCtrlFCUMapping(int16_t value) noexcept {
  HTCC::Config::SavePointCtrlFCUMapping(
    static_cast<HTCC::PointCtrlFCUMapping>(value));
}

bool MainWindow::DisableAimPointFB() const noexcept {
  return !HTCC::Config::UseHandTrackingAimPointFB;
}

void MainWindow::DisableAimPointFB(bool value) noexcept {
  HTCC::Config::SaveUseHandTrackingAimPointFB(!value);
}

bool MainWindow::ForceXRExtHandTracking() const noexcept {
  return HTCC::Config::ForceHaveXRExtHandTracking;
}

void MainWindow::ForceXRExtHandTracking(bool value) noexcept {
  HTCC::Config::SaveForceHaveXRExtHandTracking(value);
}

bool MainWindow::ForceXRFBHandTrackingAim() const noexcept {
  return HTCC::Config::ForceHaveXRFBHandTrackingAim;
}

void MainWindow::ForceXRFBHandTrackingAim(bool value) noexcept {
  HTCC::Config::SaveForceHaveXRFBHandTrackingAim(value);
}

}// namespace winrt::HTCCSettings::implementation
