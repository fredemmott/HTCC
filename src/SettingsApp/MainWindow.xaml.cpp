#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::HTCCSettings::implementation {

MainWindow::MainWindow() {
  InitializeComponent();
}

void MainWindow::Navigate(
  const IInspectable& sender,
  const Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs&
    args) noexcept {
  if (args.IsSettingsInvoked()) {
    // TODO
    return;
  }

  auto item = args.InvokedItemContainer()
                .try_as<Microsoft::UI::Xaml::Controls::NavigationViewItem>();

  if (!item) {
    // FIXME: show an error?
    return;
  }

  // TODO: you probably want to use item.Tag() to identify a specific item
  Frame().Navigate(xaml_typename<DemoPage>(), item.Content());
}

void MainWindow::GoBack(
  const IInspectable& sender,
  const Microsoft::UI::Xaml::Controls::
    NavigationViewBackRequestedEventArgs&) noexcept {
  Frame().GoBack();
}

}// namespace winrt::HTCCSettings::implementation
