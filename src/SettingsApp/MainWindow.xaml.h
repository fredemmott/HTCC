#pragma once

#include "MainWindow.g.h"
#include "pch.h"

namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::HTCCSettings::implementation {
struct MainWindow : MainWindowT<MainWindow> {
  MainWindow();

  ///// General Settings /////
  bool IsEnabled() const noexcept;
  void IsEnabled(bool) noexcept;

  int16_t PointerSource() const noexcept;
  void PointerSource(int16_t) noexcept;

  int16_t PointerSink() const noexcept;
  void PointerSink(int16_t) noexcept;

  ///// Meta Quest /////
  bool PinchToClick() const noexcept;
  void PinchToClick(bool) noexcept;

  bool PinchToScroll() const noexcept;
  void PinchToScroll(bool) noexcept;

  ///// PointCTRL /////
  void OnPointCtrlCalibrateClick(
    const IInspectable&,
    const MUX::RoutedEventArgs&);

  int16_t PointCtrlFCUMapping() const noexcept;
  void PointCtrlFCUMapping(int16_t) noexcept;

  ///// DCS /////
  int16_t MirrorEye() const noexcept;
  void MirrorEye(int16_t) noexcept;

  ///// Help/about /////

  void OnCopyVersionDataClick(const IInspectable&, const MUX::RoutedEventArgs&);

 private:
  void InitVersion();
  std::string mVersionString;
  HWND mHwnd {};
};
}// namespace winrt::HTCCSettings::implementation

namespace winrt::HTCCSettings::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}// namespace winrt::HTCCSettings::factory_implementation
