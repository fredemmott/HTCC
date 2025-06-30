// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <openxr/openxr.h>

#include <chrono>

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
    XrViewConfigurationType viewConfigurationType,
    XrTime nextDisplayTime,
    XrSpace viewSpace);

  void Update(const InputState& leftHand, const InputState& rightHand);

  static bool IsActionSink();
  static bool IsPointerSink();

  VirtualTouchScreenSink() = delete;

  static std::optional<Calibration> CalibrationFromOpenXR(
    const std::shared_ptr<OpenXRNext>& oxr,
    XrSession session,
    XrViewConfigurationType viewConfigurationType,
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
