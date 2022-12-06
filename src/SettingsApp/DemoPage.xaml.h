#pragma once

#include "DemoPage.g.h"
#include "pch.h"

namespace winrt::HTCCSettings::implementation {
struct DemoPage : DemoPageT<DemoPage> {
  DemoPage();

  void OnNavigatedTo(
    const Microsoft::UI::Xaml::Navigation::NavigationEventArgs&) noexcept;
};
}// namespace winrt::HTCCSettings::implementation

namespace winrt::HTCCSettings::factory_implementation {
struct DemoPage : DemoPageT<DemoPage, implementation::DemoPage> {};
}// namespace winrt::HTCCSettings::factory_implementation
