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

static bool IsPointerSource() {
  return Config::PointerSource == PointerSource::PointCtrl
    || Environment::IsPointCtrlCalibration;
}

PointCtrlSource::PointCtrlSource() : PointCtrlSource(NULL) {
}

PointCtrlSource::PointCtrlSource(HANDLE eventNotification)
  : mEventHandle(eventNotification) {
  DebugPrint(
    "Initializing PointCtrlSource with calibration ({}, {}) delta ({}, {})",
    Config::PointCtrlCenterX,
    Config::PointCtrlCenterY,
    Config::PointCtrlRadiansPerUnitX,
    Config::PointCtrlRadiansPerUnitY);
  DebugPrint(
    "PointerSource: {}; ActionSource: {}",
    IsPointerSource(),
    Config::PointCtrlFCUMapping != PointCtrlFCUMapping::Disabled);
  winrt::check_hresult(DirectInput8Create(
    reinterpret_cast<HINSTANCE>(&__ImageBase),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8W,
    mDI.put_void(),
    nullptr));
  ConnectDevice();
  if (!IsConnected()) {
    ConnectDeviceAsync();
  }
}

PointCtrlSource::~PointCtrlSource() {
  // avoid any destructor ordering issues
  if (mConnectDeviceThread) {
    mConnectDeviceThread->request_stop();
    mConnectDeviceThread->join();
  }
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

  if (mEventHandle) {
    winrt::check_hresult(dev->SetEventNotification(mEventHandle));
  }

  winrt::check_hresult(dev->SetDataFormat(&c_dfDIJoystick2));
  winrt::check_hresult(dev->Acquire());
  mDevice = std::move(dev);
  return DIENUM_STOP;
}

void PointCtrlSource::ConnectDeviceAsync() {
  if (mConnectDeviceThread) {
    return;
  }

  mConnectDeviceThread = std::jthread {[this](std::stop_token tok) {
    DebugPrint("Starting PointCTRL hotplug thread");
    auto lastCheck = std::chrono::steady_clock::now();
    while (!tok.stop_requested()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      const auto now = std::chrono::steady_clock::now();
      if (now - lastCheck < std::chrono::seconds(1)) {
        continue;
      }
      lastCheck = now;
      ConnectDevice();
      if (mDevice) {
        mConnectDeviceThread->detach();
        mConnectDeviceThread = {};
        DebugPrint("Terminating PointCTRL hotplug thread");
        return;
      }
    }
  }};
}

std::tuple<InputState, InputState> PointCtrlSource::Update(
  PointerMode pointerMode,
  const FrameInfo& frameInfo) {
  const auto now = frameInfo.mNow;

  if (!mDevice) {
    if (Config::PointCtrlSupportHotplug) {
      ConnectDeviceAsync();
    }
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

  auto& mX = mRaw.mX;
  auto& mY = mRaw.mY;

  if (mX != joystate.lX || mY != joystate.lY) {
    mLastMovedAt = now;
    mRaw = {};
    mX = joystate.lX;
    mY = joystate.lY;
  }

  for (auto hand: {&mLeftHand, &mRightHand}) {
    const auto b1 = HAS_BUTTON(HAND_FCUB(hand->mHand, 1));
    const auto b2 = HAS_BUTTON(HAND_FCUB(hand->mHand, 2));
    const auto b3 = HAS_BUTTON(HAND_FCUB(hand->mHand, 3));
    const auto haveButton = b1 || b2 || b3;

    if (IsPointerSource()) {
      UpdateWakeState(haveButton, now, hand);

      if (hand->mWakeState == WakeState::Waking) {
        return {{XR_HAND_LEFT_EXT}, {XR_HAND_RIGHT_EXT}};
      }
    }

    if (haveButton != hand->mHaveButton) {
      hand->mHaveButton = haveButton;
      hand->mInteractionAt = now;
    }

    hand->mState.mDirection = {};
    hand->mState.mPositionUpdatedAt = mLastMovedAt;

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

  // Left until here so we don't set these in wake state
  mRaw.mFCUL1 = HAS_BUTTON(FCUB(L1));
  mRaw.mFCUL2 = HAS_BUTTON(FCUB(L2));
  mRaw.mFCUL3 = HAS_BUTTON(FCUB(L3));
  mRaw.mFCUR1 = HAS_BUTTON(FCUB(R1));
  mRaw.mFCUR2 = HAS_BUTTON(FCUB(R2));
  mRaw.mFCUR3 = HAS_BUTTON(FCUB(R3));

  const auto interval = std::chrono::nanoseconds(now - mLastMovedAt);

  if (interval < std::chrono::milliseconds(100)) {
    const XrVector2f direction {
      (static_cast<float>(mY) - Config::PointCtrlCenterY)
        * -Config::PointCtrlRadiansPerUnitY,
      (static_cast<float>(mX) - Config::PointCtrlCenterX)
        * Config::PointCtrlRadiansPerUnitX,
    };
    mLeftHand.mState.mDirection = direction;
    mRightHand.mState.mDirection = direction;
  }

  if (mLeftHand.mInteractionAt > mRightHand.mInteractionAt) {
    return {mLeftHand.mState, {XR_HAND_RIGHT_EXT}};
  }
  return {{XR_HAND_LEFT_EXT}, mRightHand.mState};
}

void PointCtrlSource::UpdateWakeState(bool hasButtons, XrTime now, Hand* hand) {
  auto& state = hand->mWakeState;
  const auto interval = std::chrono::nanoseconds(now - hand->mInteractionAt);
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

XrTime PointCtrlSource::GetLastMovedAt() const {
  return mLastMovedAt;
}

bool PointCtrlSource::IsConnected() const {
  return static_cast<bool>(mDevice);
}

///// start button mappings /////

void PointCtrlSource::MapActionsClassic(
  Hand* hand,
  XrTime now,
  const decltype(DIJOYSTATE2::rgbButtons)& buttons) {
  auto& state = hand->mState.mActions;
  const auto b1 = HAS_BUTTON(HAND_FCUB(hand->mHand, 1));
  const auto b2 = HAS_BUTTON(HAND_FCUB(hand->mHand, 2));
  const auto b3 = HAS_BUTTON(HAND_FCUB(hand->mHand, 3));

  if (b3) {
    state.mPrimary = false;
    state.mSecondary = false;
    state.mValueChange = hand->mScrollDirection;
    return;
  }

  state.mPrimary = b1;
  state.mSecondary = b2;
  state.mValueChange = ActionState::ValueChange::None;

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
  auto& state = hand->mState.mActions;
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
          state.mSecondary = true;
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

  state.mPrimary = false;
  state.mSecondary = false;
  state.mValueChange = ActionState::ValueChange::None;
  // Set actions according to state
  switch (hand->mScrollMode) {
    case LockState::Unlocked:
      state.mPrimary = b1;
      state.mSecondary = b2;
      break;
    case LockState::MaybeLockingWithLeftHold:
      state.mPrimary = b1;
      // right click handled by state switches above
      break;
    case LockState::LockingWithLeftHoldAfterRelease:
      state.mPrimary = true;
      break;
    case LockState::SwitchingMode:
      break;
    case LockState::LockedWithLeftHold:
      state.mPrimary = true;
      [[fallthrough]];
    case LockState::LockedWithoutLeftHold:
      if (b1 && !b2) {
        state.mValueChange = ActionState::ValueChange::Decrease;
      } else if (b2 && !b1) {
        state.mValueChange = ActionState::ValueChange::Increase;
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
