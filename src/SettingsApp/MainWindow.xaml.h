#pragma once

#include "pch.h"
#include "MainWindow.g.h"

namespace winrt::DemoApp::implementation {
struct MainWindow : MainWindowT<MainWindow> {
  MainWindow();

  void Navigate(
    const IInspectable& sender,
    const Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs&) noexcept;

  void GoBack(
    const IInspectable& sender,
    const Microsoft::UI::Xaml::Controls::NavigationViewBackRequestedEventArgs&) noexcept;
};
}

namespace winrt::DemoApp::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}

