#pragma once

#include "pch.h"
#include "DemoPage.g.h"

namespace winrt::DemoApp::implementation {
struct DemoPage : DemoPageT<DemoPage> {
  DemoPage();

  void OnNavigatedTo(const Microsoft::UI::Xaml::Navigation::NavigationEventArgs&) noexcept;
};
}

namespace winrt::DemoApp::factory_implementation {
struct DemoPage : DemoPageT<DemoPage, implementation::DemoPage> {};
}

