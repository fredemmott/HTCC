#pragma once

#include "MainWindow.g.h"
#include "pch.h"

namespace winrt::HTCCSettings::implementation {
struct MainWindow : MainWindowT<MainWindow> {
  MainWindow();

  void Navigate(
    const IInspectable& sender,
    const Microsoft::UI::Xaml::Controls::
      NavigationViewItemInvokedEventArgs&) noexcept;

  void GoBack(
    const IInspectable& sender,
    const Microsoft::UI::Xaml::Controls::
      NavigationViewBackRequestedEventArgs&) noexcept;
};
}// namespace winrt::HTCCSettings::implementation

namespace winrt::HTCCSettings::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}// namespace winrt::HTCCSettings::factory_implementation
