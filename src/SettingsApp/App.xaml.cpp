#include "App.xaml.h"

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
