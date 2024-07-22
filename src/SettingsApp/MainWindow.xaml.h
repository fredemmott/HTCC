#pragma once

#include "MainWindow.g.h"
#include "pch.h"

namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::HTCCSettings::implementation {
struct MainWindow : MainWindowT<MainWindow> {
  MainWindow();

  winrt::fire_and_forget OnSponsorClick(
    const IInspectable&,
    const MUX::RoutedEventArgs&);

  ///// General Settings /////
  bool IsEnabled() const noexcept;
  void IsEnabled(bool) noexcept;

  bool IsHibernationGestureEnabled() const noexcept;
  void IsHibernationGestureEnabled(bool) noexcept;

  int16_t PointerSource() const noexcept;
  void PointerSource(int16_t) noexcept;

  bool IsPointerSourceOpenXRHandTracking() const noexcept;

  int16_t PointerSink() const noexcept;
  void PointerSink(int16_t) noexcept;

  ///// Gestures (XR_FB_hand_tracking_aim) /////
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

  ///// Workarounds /////
  bool DisableAimPointFB() const noexcept;
  void DisableAimPointFB(bool) noexcept;

  bool ForceXRExtHandTracking() const noexcept;
  void ForceXRExtHandTracking(bool) noexcept;

  bool ForceXRFBHandTrackingAim() const noexcept;
  void ForceXRFBHandTrackingAim(bool) noexcept;

  ///// Help/about /////

  void OnCopyVersionDataClick(const IInspectable&, const MUX::RoutedEventArgs&);

  /// PropertyChanged
  winrt::event_token PropertyChanged(
    const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler&);
  void PropertyChanged(const winrt::event_token&) noexcept;

 private:
  void InitVersion();
  std::string mVersionString;
  HWND mHwnd {};

  winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler>
    mPropertyChangedEvent;
};
}// namespace winrt::HTCCSettings::implementation

namespace winrt::HTCCSettings::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}// namespace winrt::HTCCSettings::factory_implementation
