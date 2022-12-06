#include "DemoPage.xaml.h"
#if __has_include("DemoPage.g.cpp")
#include "DemoPage.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.Navigation.h>

namespace winrt::HTCCSettings::implementation {

DemoPage::DemoPage() {
  InitializeComponent();
}

void DemoPage::OnNavigatedTo(
  const Microsoft::UI::Xaml::Navigation::NavigationEventArgs& args) noexcept {
  Title().Text(winrt::unbox_value<winrt::hstring>(args.Parameter()));
}

}// namespace winrt::HTCCSettings::implementation
