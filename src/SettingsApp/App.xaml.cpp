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
#include "App.xaml.h"

#include <shellapi.h>
#include <winrt/Microsoft.Windows.AppLifecycle.h>

#include <thread>

#include "DebugPrint.h"
#include "MainWindow.xaml.h"

namespace winrt::HTCCSettings::implementation {

App::App() {
  InitializeComponent();
#ifdef DEBUG
  UnhandledException(
    [this](IInspectable const&, UnhandledExceptionEventArgs const& e) {
      if (IsDebuggerPresent()) {
        auto errorMessage = e.Message();
        __debugbreak();
      }
      throw;
    });
#endif
}

void App::OnLaunched(
  Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) noexcept {
  window = make<MainWindow>();
  window.Activate();
}

}// namespace winrt::HTCCSettings::implementation

namespace {

struct EnumWindowsData {
  DWORD mProcessID {};
  HWND mHwnd {};
};

BOOL EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
  DWORD processID {};
  GetWindowThreadProcessId(hwnd, &processID);
  auto data = reinterpret_cast<EnumWindowsData*>(lParam);

  if (processID == data->mProcessID) {
    data->mHwnd = hwnd;
    return FALSE;
  }
  return TRUE;
};
}// namespace

// This currently exists just to self-elevate, as the Windows App SDK is
// currently incompatible with manifest-based elevation
int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int cmdShow) {
  {
    winrt::handle token;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.put())) {
      TOKEN_ELEVATION elevation {};
      DWORD elevationSize = sizeof(TOKEN_ELEVATION);
      if (GetTokenInformation(
            token.get(),
            TokenElevation,
            &elevation,
            sizeof(elevation),
            &elevationSize)) {
        if (!elevation.TokenIsElevated) {
          wchar_t myPath[MAX_PATH];
          GetModuleFileNameW(NULL, myPath, MAX_PATH);
          ShellExecuteW(NULL, L"runas", myPath, commandLine, nullptr, cmdShow);
          return 0;
        };
      }
    }
  }

  {
    void(WINAPI * pfnXamlCheckProcessRequirements)();
    auto module = ::LoadLibrary(L"Microsoft.ui.xaml.dll");
    if (module) {
      pfnXamlCheckProcessRequirements
        = reinterpret_cast<decltype(pfnXamlCheckProcessRequirements)>(
          GetProcAddress(module, "XamlCheckProcessRequirements"));
      if (pfnXamlCheckProcessRequirements) {
        (*pfnXamlCheckProcessRequirements)();
      }

      ::FreeLibrary(module);
    }
  }

  winrt::init_apartment(winrt::apartment_type::single_threaded);

  using AppInstance = winrt::Microsoft::Windows::AppLifecycle::AppInstance;
  auto mainInstance = AppInstance::FindOrRegisterForKey(L"main");
  if (!mainInstance.IsCurrent()) {
    HandTrackedCockpitClicking::DebugPrint(
      "Found existing instance, forwarding");
    auto args = AppInstance::GetCurrent().GetActivatedEventArgs();
    std::jthread([=]() {
      mainInstance.RedirectActivationToAsync(args).get();
      EnumWindowsData windowData {static_cast<DWORD>(mainInstance.ProcessId())};
      EnumWindows(&EnumWindowsCallback, reinterpret_cast<LPARAM>(&windowData));
      if (windowData.mHwnd) {
        SetForegroundWindow(windowData.mHwnd);
      }
    }).join();
    return 0;
  }

  ::winrt::Microsoft::UI::Xaml::Application::Start([](auto&&) {
    ::winrt::make<::winrt::HTCCSettings::implementation::App>();
  });

  return 0;
}
