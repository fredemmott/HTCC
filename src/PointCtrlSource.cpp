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

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace DirectX::SimpleMath;

namespace HandTrackedCockpitClicking {

PointCtrlSource::PointCtrlSource() {
  DebugPrint(
    "Initializing PointCtrlSource with calibration ({}, {}) delta ({}, {})",
    Config::PointCtrlCenterX,
    Config::PointCtrlCenterY,
    Config::PointCtrlRadiansPerUnitX,
    Config::PointCtrlRadiansPerUnitY);
  DebugPrint(
    "PointerSource: {}; ActionSource: {}",
    Config::PointerSource == PointerSource::PointCtrl,
    Config::PointCtrlFCUClicks);
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
    Config::PointerSource != PointerSource::PointCtrl
    && !Config::PointCtrlFCUClicks) {
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

void PointCtrlSource::Update() {
  ConnectDevice();
  if (!mDevice) {
    return;
  }

  const auto polled = mDevice->Poll();
  if (polled != DI_OK && polled != DI_NOEFFECT) {
    mDevice = {nullptr};
    return;
  }

  DIJOYSTATE2 joystate;
  if (mDevice->GetDeviceState(sizeof(joystate), &joystate) != DI_OK) {
    return;
  }

  if (mX != joystate.lX || mY != joystate.lY) {
    mLastMovedAt = std::chrono::steady_clock::now();
    mX = joystate.lX;
    mY = joystate.lY;
  }

  ActionState newState;
  if (Config::PointCtrlFCUMapping == PointCtrlFCUMapping::Classic) {
    MapActionsClassic(newState, joystate.rgbButtons);
  } else if (Config::PointCtrlFCUMapping == PointCtrlFCUMapping::MSFS) {
    MapActionsMSFS(newState, joystate.rgbButtons);
  }

  if (Config::VerboseDebug >= 1 && newState != mActionState) {
    DebugPrint(
      "FCU button state change: L {} R {} U {} D {}; scroll lock {}",
      newState.mLeftClick,
      newState.mRightClick,
      newState.mWheelUp,
      newState.mWheelDown,
      static_cast<uint8_t>(mScrollMode));
  }

  mActionState = newState;
}

ActionState PointCtrlSource::GetActionState() const {
  return mActionState;
}

std::tuple<uint16_t, uint16_t>
PointCtrlSource::GetRawCoordinatesForCalibration() const {
  return {mX, mY};
}

bool PointCtrlSource::IsConnected() const {
  return static_cast<bool>(mDevice);
}

bool PointCtrlSource::IsStale() const {
  return std::chrono::steady_clock::now() - mLastMovedAt
    >= std::chrono::seconds(1);
}

std::optional<XrVector2f> PointCtrlSource::GetRXRY() const {
  if (IsStale()) {
    return {};
  }

  return {{
    (static_cast<float>(mY) - Config::PointCtrlCenterY)
      * -Config::PointCtrlRadiansPerUnitY,
    (static_cast<float>(mX) - Config::PointCtrlCenterX)
      * Config::PointCtrlRadiansPerUnitX,
  }};
}

std::tuple<std::optional<XrPosef>, std::optional<XrPosef>>
PointCtrlSource::GetPoses() const {
  auto rotations = GetRXRY();
  if (!rotations) {
    return {{}, {}};
  }

  const auto [rx, ry] = *rotations;
  const auto leftHand = ry < 0;

  const auto pointDirection
    = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);

  const auto p = Vector3::Transform(
    {0.0f, 0.0f, -Config::PointCtrlProjectionDistance}, pointDirection);
  const auto o = pointDirection;

  XrPosef pose {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };

  if (leftHand) {
    return {{pose}, {}};
  }
  return {{}, {pose}};
}

///// start button mappings /////

static constexpr auto pressedBit = 1 << 7;
#define FCUB(x) Config::PointCtrlFCUButton##x
#define HAS_BUTTON(idx) ((buttons[idx] & pressedBit) == pressedBit)
#define HAS_EITHER_BUTTON(a, b) (HAS_BUTTON(a) || HAS_BUTTON(b))

void PointCtrlSource::MapActionsClassic(
  ActionState& newState,
  const decltype(DIJOYSTATE2::rgbButtons)& buttons) {
  newState = {
    .mLeftClick = HAS_EITHER_BUTTON(FCUB(L1), FCUB(R1)),
    .mRightClick = HAS_EITHER_BUTTON(FCUB(L2), FCUB(R2)),
  };
  if (newState.mLeftClick && !newState.mRightClick) {
    mScrollDirection = ScrollDirection::Down;
  }
  if (newState.mRightClick && !newState.mLeftClick) {
    mScrollDirection = ScrollDirection::Up;
  }

  if (HAS_EITHER_BUTTON(FCUB(L3), FCUB(R3))) {
    newState.mWheelDown = (mScrollDirection == ScrollDirection::Down);
    newState.mWheelUp = (mScrollDirection == ScrollDirection::Up);
  }
}

void PointCtrlSource::MapActionsMSFS(
  ActionState& newState,
  const decltype(DIJOYSTATE2::rgbButtons)& buttons) {
  const auto previousScrollMode = mScrollMode;

  const auto fcu1 = HAS_EITHER_BUTTON(FCUB(L1), FCUB(R1));
  const auto fcu2 = HAS_EITHER_BUTTON(FCUB(L2), FCUB(R2));
  const auto fcu3 = HAS_EITHER_BUTTON(FCUB(L3), FCUB(R3));

  const auto now = std::chrono::steady_clock::now();

  // Update state
  switch (mScrollMode) {
    case LockState::Unlocked:
      if (fcu1 && fcu2) {
        mScrollMode = LockState::MaybeLockingWithLeftHold;
        mModeSwitchStart = now;
      } else if (fcu3) {
        mScrollMode = LockState::SwitchingMode;
        mModeSwitchStart = now;
      }
      break;
    case LockState::MaybeLockingWithLeftHold:
      if (!fcu2) {
        if (now - mModeSwitchStart > std::chrono::milliseconds(200)) {
          mScrollMode = LockState::LockingWithLeftHoldAfterRelease;
        } else {
          mScrollMode = LockState::Unlocked;
          // will be unset on next frame
          newState.mRightClick = true;
        }
      } else if (!fcu1) {
        mScrollMode = LockState::LockingWithLeftHoldAfterRelease;
      }
      break;
    case LockState::SwitchingMode:
      if (!fcu3) {
        if (now - mModeSwitchStart > std::chrono::milliseconds(200)) {
          mScrollMode = LockState::LockedWithoutLeftHold;
        } else {
          mScrollMode = LockState::Unlocked;
        }
      }
      break;
    case LockState::LockingWithLeftHoldAfterRelease:
      if (!(fcu1 || fcu2)) {
        mScrollMode = LockState::LockedWithLeftHold;
      }
      break;
    case LockState::LockedWithLeftHold:
    case LockState::LockedWithoutLeftHold:
      if (fcu3) {
        mScrollMode = LockState::SwitchingMode;
        mModeSwitchStart = now;
      }
      break;
  }

  // Set actions according to state
  switch (mScrollMode) {
    case LockState::Unlocked:
      newState.mRightClick = fcu2;
      [[fallthrough]];
    case LockState::MaybeLockingWithLeftHold:
      newState.mLeftClick = fcu1;
      // right click handled by state switches
      break;
    case LockState::SwitchingMode:
    case LockState::LockingWithLeftHoldAfterRelease:
      break;
    case LockState::LockedWithLeftHold:
      newState = {
        .mLeftClick = true,
        .mWheelUp = fcu1,
        .mWheelDown = fcu2,
      };
      break;
    case LockState::LockedWithoutLeftHold:
      newState = {
        .mWheelUp = fcu1,
        .mWheelDown = fcu2,
      };
      break;
  }

  if (mScrollMode != previousScrollMode && Config::VerboseDebug >= 1) {
    DebugPrint(
      "Scroll mode change: {} -> {}",
      static_cast<uint8_t>(previousScrollMode),
      static_cast<uint8_t>(mScrollMode));
  }
}

///// end button mappings /////

}// namespace HandTrackedCockpitClicking
