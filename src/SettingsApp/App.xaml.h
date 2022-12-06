#pragma once

#include "App.xaml.g.h"
#include "pch.h"

namespace winrt::HTCCSettings::implementation {
struct App : AppT<App> {
  App();

  void OnLaunched(
    Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) noexcept;

 private:
  winrt::Microsoft::UI::Xaml::Window window {nullptr};
};

}// namespace winrt::HTCCSettings::implementation
