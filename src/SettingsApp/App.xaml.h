#pragma once

#include "pch.h"
#include "App.xaml.g.h"

namespace winrt::DemoApp::implementation {
struct App : AppT<App> {
  App();

  void OnLaunched(
    Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) noexcept;

 private:
  winrt::Microsoft::UI::Xaml::Window window {nullptr};
};

}
