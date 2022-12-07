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
#include "PointCtrlSource.h"

#include <directxtk/SimpleMath.h>

#include <numbers>

#include "Config.h"
#include "DebugPrint.h"
#include "Environment.h"
#include "openxr.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace DirectX::SimpleMath;

static constexpr auto PressedBit = 1 << 7;
#define FCUB(x) Config::PointCtrlFCUButton##x
#define HAS_BUTTON(idx) ((buttons[idx] & PressedBit) == PressedBit)
#define HAND_FCUB(hand, x) (hand == XR_HAND_LEFT_EXT ? FCUB(L##x) : FCUB(R##x))

namespace HandTrackedCockpitClicking {

PointCtrlSource::PointCtrlSource(
  const std::shared_ptr<OpenXRNext>& next,
  XrInstance instance,
  XrSession session,
  XrSpace viewSpace,
  XrSpace localSpace)
  : mInstance(instance),
    mSession(session),
    mViewSpace(viewSpace),
    mLocalSpace(localSpace),
    mOpenXR(next) {
  DebugPrint(
    "Initializing PointCtrlSource with calibration ({}, {}) delta ({}, {})",
    Config::PointCtrlCenterX,
    Config::PointCtrlCenterY,
    Config::PointCtrlRadiansPerUnitX,
    Config::PointCtrlRadiansPerUnitY);
  DebugPrint(
    "PointerSource: {}; ActionSource: {}",
    Config::PointerSource == PointerSource::PointCtrl,
    Config::PointCtrlFCUMapping != PointCtrlFCUMapping::Disabled);
  winrt::check_hresult(DirectInput8Create(
    reinterpret_cast<HINSTANCE>(&__ImageBase),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8W,
    mDI.put_void(),
    nullptr));
  ConnectDevice();
}

void PointCtrlSource::ConnectDevice() {
  if (mDevice) {
    return;
  }

  // If we're not going to do anything with it, don't fetch the data.
  if (
    (!IsPointerSource())
    && Config::PointCtrlFCUMapping == PointCtrlFCUMapping::Disabled) {
    return;
  }

  // Don't check for device every frame
  static std::chrono::steady_clock::time_point sLastCheck {};
  const auto now = std::chrono::steady_clock::now();
  if (now - sLastCheck < std::chrono::seconds(1)) {
    return;
  }
  sLastCheck = now;

  winrt::check_hresult(mDI->EnumDevices(
    DI8DEVCLASS_GAMECTRL,
    &PointCtrlSource::EnumDevicesCallbackStatic,
    this,
    DIEDFL_ATTACHEDONLY));
}

BOOL PointCtrlSource::EnumDevicesCallbackStatic(
  LPCDIDEVICEINSTANCE lpddi,
  LPVOID pvRef) {
  return reinterpret_cast<PointCtrlSource*>(pvRef)->EnumDevicesCallback(lpddi);
}

BOOL PointCtrlSource::EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi) {
  winrt::com_ptr<IDirectInputDevice8W> dev;
  winrt::check_hresult(
    mDI->CreateDevice(lpddi->guidInstance, dev.put(), nullptr));

  DIPROPDWORD buf;
  buf.diph.dwSize = sizeof(DIPROPDWORD);
  buf.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  buf.diph.dwObj = 0;
  buf.diph.dwHow = DIPH_DEVICE;

  winrt::check_hresult(dev->GetProperty(DIPROP_VIDPID, &buf.diph));

  const auto vid = LOWORD(buf.dwData);
  const auto pid = HIWORD(buf.dwData);

  if (vid != Config::PointCtrlVID || pid != Config::PointCtrlPID) {
    return DIENUM_CONTINUE;
  }

  DebugPrint(L"Found PointCtrlDevice '{}'", lpddi->tszInstanceName);

  winrt::check_hresult(dev->SetDataFormat(&c_dfDIJoystick2));
  winrt::check_hresult(dev->Acquire());
  mDevice = dev;
  return DIENUM_STOP;
}

void PointCtrlSource::UpdatePose(const FrameInfo& frameInfo, InputState* hand) {
  hand->mPose = {};
  if (!hand->mDirection) {
    return;
  }
  const auto rx = hand->mDirection->x;
  const auto ry = hand->mDirection->y;

  const auto pointDirection
    = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);

  const auto p = Vector3::Transform(
    {0.0f, 0.0f, -Config::PointCtrlProjectionDistance}, pointDirection);
  const auto o = pointDirection;

  const XrPosef viewPose {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };

  const auto worldPose = viewPose * frameInfo.mViewInLocal;
  hand->mPose = worldPose;
}

std::tuple<InputState, InputState> PointCtrlSource::Update(
  PointerMode pointerMode,
  const FrameInfo& frameInfo) {
  const auto now = frameInfo.mNow;

  ConnectDevice();
  if (!mDevice) {
    return {{XR_HAND_LEFT_EXT}, {XR_HAND_RIGHT_EXT}};
  }

  const auto polled = mDevice->Poll();
  if (polled != DI_OK && polled != DI_NOEFFECT) {
    mDevice = {nullptr};
    return {{XR_HAND_LEFT_EXT}, {XR_HAND_RIGHT_EXT}};
  }

  DIJOYSTATE2 joystate;
  if (mDevice->GetDeviceState(sizeof(joystate), &joystate) != DI_OK) {
    return {{XR_HAND_LEFT_EXT}, {XR_HAND_RIGHT_EXT}};
  }
  const auto& buttons = joystate.rgbButtons;

  if (
    Config::PointerSource == PointerSource::PointCtrl
    || Environment::IsPointCtrlCalibration) {
    const auto anyLeftButton
      = HAS_BUTTON(FCUB(L1)) || HAS_BUTTON(FCUB(L2)) || HAS_BUTTON(FCUB(L3));
    const auto anyRightButton
      = HAS_BUTTON(FCUB(R1)) || HAS_BUTTON(FCUB(R2)) || HAS_BUTTON(FCUB(R3));
  }
  auto& mX = mRaw.mX;
  auto& mY = mRaw.mY;

  if (mX != joystate.lX || mY != joystate.lY) {
    mLastMovedAt = now;
    mX = joystate.lX;
    mY = joystate.lY;
    mRaw.mFCUL1 = HAS_BUTTON(FCUB(L1));
    mRaw.mFCUL2 = HAS_BUTTON(FCUB(L2));
    mRaw.mFCUL3 = HAS_BUTTON(FCUB(L3));
    mRaw.mFCUR1 = HAS_BUTTON(FCUB(R1));
    mRaw.mFCUR2 = HAS_BUTTON(FCUB(R2));
    mRaw.mFCUR3 = HAS_BUTTON(FCUB(R3));
  }

  for (auto hand: {&mLeftHand, &mRightHand}) {
    const auto b1 = HAS_BUTTON(HAND_FCUB(hand->mHand, 1));
    const auto b2 = HAS_BUTTON(HAND_FCUB(hand->mHand, 2));
    const auto b3 = HAS_BUTTON(HAND_FCUB(hand->mHand, 3));
    const auto haveButton = b1 || b2 || b3;

    if (Config::PointerSource == PointerSource::PointCtrl) {
      UpdateWakeState(haveButton, now, hand);

      if (hand->mWakeState == WakeState::Waking) {
        return {{XR_HAND_LEFT_EXT}, {XR_HAND_RIGHT_EXT}};
      }
    }

    if (haveButton != hand->mHaveButton) {
      hand->mHaveButton = haveButton;
      hand->mInteractionAt = now;
    }

    hand->mState.mPose = {};
    hand->mState.mDirection = {};
    hand->mState.mUpdatedAt = std::max(mLastMovedAt, hand->mInteractionAt);

    switch (Config::PointCtrlFCUMapping) {
      case PointCtrlFCUMapping::Classic:
        MapActionsClassic(hand, now, buttons);
        break;
      case PointCtrlFCUMapping::Modal:
      case PointCtrlFCUMapping::ModalWithLeftLock:
        MapActionsModal(hand, now, buttons);
        break;
    }
  }

  const auto interval = std::chrono::nanoseconds(now - mLastMovedAt);
  if (
    pointerMode == PointerMode::None
    || interval > std::chrono::milliseconds(200)) {
    if (mLeftHand.mInteractionAt > mRightHand.mInteractionAt) {
      return {mLeftHand.mState, {XR_HAND_RIGHT_EXT}};
    }
    return {{XR_HAND_LEFT_EXT}, mRightHand.mState};
  }

  const XrVector2f direction {
    (static_cast<float>(mY) - Config::PointCtrlCenterY)
      * -Config::PointCtrlRadiansPerUnitY,
    (static_cast<float>(mX) - Config::PointCtrlCenterX)
      * Config::PointCtrlRadiansPerUnitX,
  };

  mLeftHand.mState.mUpdatedAt = now;
  mRightHand.mState.mUpdatedAt = now;
  if (mLeftHand.mInteractionAt > mRightHand.mInteractionAt) {
    mLeftHand.mState.mDirection = direction;
    if (pointerMode == PointerMode::Pose) {
      UpdatePose(frameInfo, &mLeftHand.mState);
    }

    return {mLeftHand.mState, {XR_HAND_RIGHT_EXT}};
  }

  mRightHand.mState.mDirection = direction;
  if (pointerMode == PointerMode::Pose) {
    UpdatePose(frameInfo, &mRightHand.mState);
  }
  return {{XR_HAND_LEFT_EXT}, mRightHand.mState};
}

void PointCtrlSource::UpdateWakeState(bool hasButtons, XrTime now, Hand* hand) {
  auto& state = hand->mWakeState;
  const auto interval = std::chrono::nanoseconds(now = hand->mInteractionAt);
  if (state == WakeState::Default && hasButtons) {
    if (
      interval
      > std::chrono::milliseconds(Config::PointCtrlSleepMilliseconds)) {
      state = WakeState::Waking;
    }
    hand->mInteractionAt = now;
    return;
  }
  if (state == WakeState::Waking && !hasButtons) {
    hand->mInteractionAt = now;
    state = WakeState::Default;
    return;
  }

  if (hasButtons) {
    hand->mInteractionAt = now;
  }
}

PointCtrlSource::RawValues PointCtrlSource::GetRawValuesForCalibration() const {
  return mRaw;
}

bool PointCtrlSource::IsConnected() const {
  return static_cast<bool>(mDevice);
}

///// start button mappings /////

void PointCtrlSource::MapActionsClassic(
  Hand* hand,
  XrTime now,
  const decltype(DIJOYSTATE2::rgbButtons)& buttons) {
  auto& state = hand->mState;
  const auto b1 = HAS_BUTTON(HAND_FCUB(hand->mHand, 1));
  const auto b2 = HAS_BUTTON(HAND_FCUB(hand->mHand, 2));
  const auto b3 = HAS_BUTTON(HAND_FCUB(hand->mHand, 3));

  if (b3) {
    state.mPrimaryInteraction = false;
    state.mSecondaryInteraction = false;
    state.mValueChange = hand->mScrollDirection;
    return;
  }

  state.mPrimaryInteraction = b1;
  state.mSecondaryInteraction = b2;
  state.mValueChange = InputState::ValueChange::None;

  if (b1 && !b2) {
    hand->mScrollDirection = ScrollDirection::Increase;
  } else if (b2 && !b1) {
    hand->mScrollDirection = ScrollDirection::Decrease;
  }
}

void PointCtrlSource::MapActionsModal(
  Hand* hand,
  XrTime now,
  const decltype(DIJOYSTATE2::rgbButtons)& buttons) {
  auto& state = hand->mState;
  const auto b1 = HAS_BUTTON(HAND_FCUB(hand->mHand, 1));
  const auto b2 = HAS_BUTTON(HAND_FCUB(hand->mHand, 2));
  const auto b3 = HAS_BUTTON(HAND_FCUB(hand->mHand, 3));

  const auto previousValueChange = state.mValueChange;

  const auto interval = std::chrono::nanoseconds(now - hand->mModeSwitchStart);

  // Update state
  switch (hand->mScrollMode) {
    case LockState::Unlocked:
      if (
        b1 && b2
        && Config::PointCtrlFCUMapping
          == PointCtrlFCUMapping::ModalWithLeftLock) {
        hand->mScrollMode = LockState::MaybeLockingWithLeftHold;
        hand->mModeSwitchStart = now;
      } else if (b3) {
        hand->mScrollMode = LockState::SwitchingMode;
        hand->mModeSwitchStart = now;
      }
      break;
    case LockState::MaybeLockingWithLeftHold:
      if (!b2) {
        if (
          interval > std::chrono::milliseconds(
            Config::ShortPressLongPressMilliseconds)) {
          hand->mScrollMode = LockState::LockingWithLeftHoldAfterRelease;
        } else {
          hand->mScrollMode = LockState::Unlocked;
          // will be unset on next frame
          state.mSecondaryInteraction = true;
        }
      } else if (!b1) {
        hand->mScrollMode = LockState::LockingWithLeftHoldAfterRelease;
      }
      break;
    case LockState::SwitchingMode:
      if (!b3) {
        if (
          interval > std::chrono::milliseconds(
            Config::ShortPressLongPressMilliseconds)) {
          hand->mScrollMode = LockState::LockedWithoutLeftHold;
        } else {
          hand->mScrollMode = LockState::Unlocked;
        }
      }
      break;
    case LockState::LockingWithLeftHoldAfterRelease:
      if (!(b1 || b2)) {
        hand->mScrollMode = LockState::LockedWithLeftHold;
      }
      break;
    case LockState::LockedWithLeftHold:
    case LockState::LockedWithoutLeftHold:
      if (b3) {
        hand->mScrollMode = LockState::SwitchingMode;
        hand->mModeSwitchStart = now;
      }
      break;
  }

  state.mPrimaryInteraction = false;
  state.mSecondaryInteraction = false;
  state.mValueChange = InputState::ValueChange::None;
  // Set actions according to state
  switch (hand->mScrollMode) {
    case LockState::Unlocked:
      state.mPrimaryInteraction = b1;
      state.mSecondaryInteraction = b2;
      break;
    case LockState::MaybeLockingWithLeftHold:
      state.mPrimaryInteraction = b1;
      // right click handled by state switches above
      break;
    case LockState::LockingWithLeftHoldAfterRelease:
      state.mPrimaryInteraction = true;
      break;
    case LockState::SwitchingMode:
      break;
    case LockState::LockedWithLeftHold:
      state.mPrimaryInteraction = true;
      [[fallthrough]];
    case LockState::LockedWithoutLeftHold:
      if (b1 && !b2) {
        state.mValueChange = InputState::ValueChange::Decrease;
      } else if (b2 && !b1) {
        state.mValueChange = InputState::ValueChange::Increase;
      }
      break;
  }

  if (state.mValueChange != previousValueChange && Config::VerboseDebug >= 1) {
    DebugPrint(
      "Scroll mode change: {} -> {}",
      static_cast<uint8_t>(previousValueChange),
      static_cast<uint8_t>(state.mValueChange));
  }
}

///// end button mappings /////

}// namespace HandTrackedCockpitClicking
