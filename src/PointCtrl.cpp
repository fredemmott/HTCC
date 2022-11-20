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
#include "PointCtrl.h"

#include "Config.h"
#include "DebugPrint.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace DCSQuestHandTracking {

PointCtrl::PointCtrl() {
  winrt::check_hresult(DirectInput8Create(
    reinterpret_cast<HINSTANCE>(&__ImageBase),
    DIRECTINPUT_VERSION,
    IID_IDirectInput8W,
    mDI.put_void(),
    nullptr));

  winrt::check_hresult(mDI->EnumDevices(
    DI8DEVCLASS_GAMECTRL,
    &PointCtrl::EnumDevicesCallbackStatic,
    this,
    DIEDFL_ATTACHEDONLY));
}

BOOL PointCtrl::EnumDevicesCallbackStatic(
  LPCDIDEVICEINSTANCE lpddi,
  LPVOID pvRef) {
  return reinterpret_cast<PointCtrl*>(pvRef)->EnumDevicesCallback(lpddi);
}

BOOL PointCtrl::EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi) {
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

  if (vid != PointCtrl::VID || pid != PointCtrl::PID) {
    return DIENUM_CONTINUE;
  }

  DebugPrint(L"Found PointCtrlDevice '{}'", lpddi->tszInstanceName);

  winrt::check_hresult(dev->SetDataFormat(&c_dfDIJoystick2));
  winrt::check_hresult(dev->Acquire());
  mDevice = dev;
  return DIENUM_STOP;
}

std::optional<ActionState> PointCtrl::GetActionState() {
  if (!mDevice) {
    return {};
  }

  const auto polled = mDevice->Poll();
  if (polled != DI_OK && polled != DI_NOEFFECT) {
    return {mActionState};
  }

  DIJOYSTATE2 joystate;
  if (mDevice->GetDeviceState(sizeof(joystate), &joystate) != DI_OK) {
    return {mActionState};
  }

  constexpr auto pressedBit = 1 << 7;
  const auto& buttons = joystate.rgbButtons;

  ActionState newState {
    .mLeftClick = (buttons[LeftClickButton] & pressedBit) == pressedBit,
    .mRightClick = (buttons[RightClickButton] & pressedBit) == pressedBit,
    .mWheelUp = (buttons[WheelUpButton] & pressedBit) == pressedBit,
    .mWheelDown = (buttons[WheelDownButton] & pressedBit) == pressedBit,
  };

  if (Config::VerboseDebug >= 2 && newState != mActionState) {
    DebugPrint(
      "PointCtrl FCU button state change: L {} R {} U {} D {}",
      newState.mLeftClick,
      newState.mRightClick,
      newState.mWheelUp,
      newState.mWheelDown);
  }

  mActionState = newState;

  return {mActionState};
}

}// namespace DCSQuestHandTracking
