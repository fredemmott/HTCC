// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <dinput.h>
#include <openxr/openxr.h>

#include <cinttypes>
#include <thread>

#include "InputSource.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

// Wrapper for PointCtrl joystick devices. This currently requires a custom
// firmware.
class PointCtrlSource final : public InputSource {
 public:
  explicit PointCtrlSource(HANDLE eventNotification);
  PointCtrlSource();
  ~PointCtrlSource();

  bool IsConnected() const;
  std::tuple<InputState, InputState> Update(PointerMode, const FrameInfo&)
    override;

  // Just used for calibration
  struct RawValues {
    uint16_t mX {};
    uint16_t mY {};
    bool mFCUL1 {false};
    bool mFCUL2 {false};
    bool mFCUL3 {false};
    bool mFCUR1 {false};
    bool mFCUR2 {false};
    bool mFCUR3 {false};

    constexpr bool FCU1() const {
      return mFCUL1 || mFCUR1;
    }
    constexpr bool FCU2() const {
      return mFCUL2 || mFCUR2;
    }
    constexpr bool FCU3() const {
      return mFCUL3 || mFCUR3;
    }
  };
  RawValues GetRawValuesForCalibration() const;
  XrTime GetLastMovedAt() const;

 private:
  std::optional<std::jthread> mConnectDeviceThread;
  void ConnectDevice();
  void ConnectDeviceAsync();

  static BOOL EnumDevicesCallbackStatic(
    LPCDIDEVICEINSTANCE lpddi,
    LPVOID pvRef);
  BOOL EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi);

  enum class LockState {
    Unlocked,
    MaybeLockingWithLeftHold,
    SwitchingMode,
    LockingWithLeftHoldAfterRelease,
    LockedWithLeftHold,
    LockedWithoutLeftHold,
  };
  enum class WakeState {
    Default,
    Waking,
  };
  using ScrollDirection = ActionState::ValueChange;
  struct Hand {
    XrHandEXT mHand {};
    InputState mState {};

    WakeState mWakeState {WakeState::Default};

    LockState mScrollMode {LockState::Unlocked};
    ScrollDirection mScrollDirection {ScrollDirection::Increase};
    XrTime mModeSwitchStart {};
    XrTime mInteractionAt {};
    bool mHaveButton {false};
  };
  Hand mLeftHand {XR_HAND_LEFT_EXT, {XR_HAND_LEFT_EXT}};
  Hand mRightHand {XR_HAND_RIGHT_EXT, {XR_HAND_RIGHT_EXT}};

  winrt::com_ptr<IDirectInput8W> mDI;
  winrt::com_ptr<IDirectInputDevice8W> mDevice;
  HANDLE mEventHandle {};

  RawValues mRaw {};
  XrTime mLastMovedAt {};

  void UpdatePose(const FrameInfo&, InputState* hand);
  void UpdateWakeState(bool hasButtons, XrTime now, Hand* hand);

  using RawButtons = decltype(DIJOYSTATE2::rgbButtons);

  void MapActionsClassic(Hand*, XrTime now, const RawButtons&);
  void MapActionsModal(Hand*, XrTime now, const RawButtons&);
  void MapActionsDedicatedScrollButtons(Hand*, XrTime now, const RawButtons&);
};

}// namespace HandTrackedCockpitClicking
