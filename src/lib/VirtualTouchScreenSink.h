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

#include <openxr/openxr.h>

#include "Config.h"
#include "InputState.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class VirtualTouchScreenSink final {
 public:
  struct Calibration {
    XrVector2f mWindowInputFov {};
    XrVector2f mWindowInputFovOrigin0To1 {};
  };

  VirtualTouchScreenSink(std::optional<Calibration>, DWORD targetProcessID);
  VirtualTouchScreenSink(
    const std::shared_ptr<OpenXRNext>& oxr,
    XrSession session,
    XrTime nextDisplayTime,
    XrSpace viewSpace);

  void Update(const InputState& leftHand, const InputState& rightHand);

  static bool IsActionSink();
  static bool IsPointerSink();

  VirtualTouchScreenSink() = delete;

  static std::optional<Calibration> CalibrationFromOpenXR(
    const std::shared_ptr<OpenXRNext>& oxr,
    XrSession session,
    XrTime nextDisplayTime,
    XrSpace viewSpace);

  static Calibration CalibrationFromOpenXRView(const XrView& view);
  static std::optional<Calibration> CalibrationFromConfig();

 private:
  void Update(const InputState& hand);
  bool RotationToCartesian(const XrVector2f& rotation, XrVector2f* cartesian);
  void UpdateMainWindow();
  static BOOL CALLBACK EnumWindowCallback(HWND hwnd, LPARAM lparam);

  DWORD mTargetProcessID;
  HWND mWindow {};
  HWND mConsoleWindow {};
  XrVector2f mWindowSize {};
  RECT mWindowRect {};
  XrVector2f mScreenSize;

  std::optional<Calibration> mCalibration {};

  bool mLeftClick {false};
  bool mRightClick {false};
  ActionState::ValueChange mScrollDirection = ActionState::ValueChange::None;

  std::chrono::steady_clock::time_point mLastWindowCheck {};

  std::chrono::steady_clock::time_point mNextScrollEvent {};
};

}// namespace HandTrackedCockpitClicking
