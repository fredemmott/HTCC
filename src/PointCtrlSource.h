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

#include <chrono>
#include <cinttypes>

#include "ActionState.h"

namespace DCSQuestHandTracking {

// Wrapper for PointCtrl joystick devices. This currently requires a custom
// firmware.
class PointCtrlSource final {
 public:
  PointCtrlSource();

  void Update();

  std::optional<XrVector2f> GetRXRY() const;
  ActionState GetActionState() const;
  std::tuple<uint16_t, uint16_t> GetRawCoordinatesForCalibration() const;
  std::tuple<std::optional<XrPosef>, std::optional<XrPosef>> GetPoses() const;

 private:
  bool IsStale() const;
  static BOOL EnumDevicesCallbackStatic(
    LPCDIDEVICEINSTANCE lpddi,
    LPVOID pvRef);
  BOOL EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi);

  static constexpr uint16_t VID {0x04d8};
  static constexpr uint16_t PID {0xeeec};

  // 0-indexed; most visualizers are 1-indexed
  static constexpr uint8_t LeftClickButton = 0;
  static constexpr uint8_t RightClickButton = 1;
  static constexpr uint8_t WheelUpButton = 9;
  static constexpr uint8_t WheelDownButton = 10;

  winrt::com_ptr<IDirectInput8W> mDI;
  winrt::com_ptr<IDirectInputDevice8W> mDevice;

  ActionState mActionState {};

  uint16_t mX {};
  uint16_t mY {};
  std::chrono::steady_clock::time_point mLastMovedAt {};
};

}// namespace DCSQuestHandTracking
