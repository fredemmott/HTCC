/*
 * MIT License
 *
 * Copyright (c) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include <dinput.h>
#include <openxr/openxr.h>

#include <cinttypes>

#include "InputSource.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

// Wrapper for PointCtrl joystick devices. This currently requires a custom
// firmware.
class PointCtrlSource final : public InputSource {
 public:
  PointCtrlSource(
    const std::shared_ptr<OpenXRNext>& next,
    XrInstance instance,
    XrSession session,
    XrSpace viewSpace,
    XrSpace localSpace);

  bool IsConnected() const;

  std::tuple<InputState, InputState> Update(PointerMode, const FrameInfo&)
    override;

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
  void ConnectDevice();
  bool IsStale() const;
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
  using ScrollDirection = InputState::ValueChange;
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

  RawValues mRaw {};
  XrTime mLastMovedAt {};

  XrInstance mInstance {};
  XrSession mSession {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};
  std::shared_ptr<OpenXRNext> mOpenXR;

  void UpdatePose(const FrameInfo&, InputState* hand);
  void UpdateWakeState(bool hasButtons, XrTime now, Hand* hand);
  void MapActionsClassic(
    Hand*,
    XrTime now,
    const decltype(DIJOYSTATE2::rgbButtons)&);
  void
  MapActionsModal(Hand*, XrTime now, const decltype(DIJOYSTATE2::rgbButtons)&);
};

}// namespace HandTrackedCockpitClicking
