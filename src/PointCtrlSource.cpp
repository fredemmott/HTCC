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

  if (vid != PointCtrlSource::VID || pid != PointCtrlSource::PID) {
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

  constexpr auto pressedBit = 1 << 7;
  const auto& buttons = joystate.rgbButtons;
#define HAS_BUTTON(idx) ((buttons[idx] & pressedBit) == pressedBit)
#define HAS_EITHER_BUTTON(a, b) (HAS_BUTTON(a) || HAS_BUTTON(b))
  ActionState newState {
    .mLeftClick = HAS_EITHER_BUTTON(FCUButton::L1, FCUButton::R1),
    .mRightClick = HAS_EITHER_BUTTON(FCUButton::L2, FCUButton::R2),
  };
  if (newState.mLeftClick && !newState.mRightClick) {
    mScrollDirection = ScrollDirection::Down;
  }
  if (newState.mRightClick && !newState.mLeftClick) {
    mScrollDirection = ScrollDirection::Up;
  }

  if (HAS_EITHER_BUTTON(FCUButton::L3, FCUButton::R3)) {
    newState.mWheelDown = (mScrollDirection == ScrollDirection::Down);
    newState.mWheelUp = (mScrollDirection == ScrollDirection::Up);
  }
#undef HAS_BUTTON
#undef HAS_EITHER_BUTTON

  if (Config::VerboseDebug >= 2 && newState != mActionState) {
    DebugPrint(
      "PointCtrl FCU button state change: L {} R {} U {} D {}",
      newState.mLeftClick,
      newState.mRightClick,
      newState.mWheelUp,
      newState.mWheelDown);
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
}// namespace HandTrackedCockpitClicking
