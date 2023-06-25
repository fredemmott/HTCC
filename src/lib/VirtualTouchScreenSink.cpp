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
#include "VirtualTouchScreenSink.h"

#include <cmath>

#include "Config.h"
#include "DebugPrint.h"

namespace HandTrackedCockpitClicking {

VirtualTouchScreenSink::VirtualTouchScreenSink(
  Calibration calibration,
  DWORD targetProcessID)
  : mCalibration(calibration), mTargetProcessID(targetProcessID) {
  DebugPrint(
    "Initialized virtual touch screen - PointerSink: {}; ActionSink: {}",
    IsPointerSink(),
    IsActionSink());

  UpdateMainWindow();
}

VirtualTouchScreenSink::VirtualTouchScreenSink(
  const std::shared_ptr<OpenXRNext>& oxr,
  XrSession session,
  XrTime nextDisplayTime,
  XrSpace viewSpace)
  : VirtualTouchScreenSink(
    CalibrationFromOpenXR(oxr, session, nextDisplayTime, viewSpace).value(),
    GetCurrentProcessId()) {
}

std::optional<VirtualTouchScreenSink::Calibration>
VirtualTouchScreenSink::CalibrationFromOpenXR(
  const std::shared_ptr<OpenXRNext>& oxr,
  XrSession session,
  XrTime nextDisplayTime,
  XrSpace viewSpace) {
  XrViewLocateInfo viewLocateInfo {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    .displayTime = nextDisplayTime,
    .space = viewSpace,
  };
  XrViewState viewState {XR_TYPE_VIEW_STATE};
  uint32_t viewCount {};
  if (!oxr->check_xrLocateViews(
        session,
        &viewLocateInfo,
        &viewState,
        /* view count = */ 0,
        &viewCount,
        nullptr)) {
    DebugPrint("Failed to get number of views.");
    return {};
  }
  if (viewCount == 0) {
    DebugPrint("View count is 0");
    return {};
  }

  std::vector<XrView> views;
  views.resize(viewCount, {XR_TYPE_VIEW});

  if (!oxr->check_xrLocateViews(
        session,
        &viewLocateInfo,
        &viewState,
        views.size(),
        &viewCount,
        views.data())) {
    DebugPrint("Failed to find FOV");
    return {};
  }

  auto calibration = CalibrationFromOpenXRView(views[0]);

  DebugPrint(
    "Reported eye FOV: {}x{} - tracking origin "
    "at ({}, {})",
    calibration.mWindowInputFov.x,
    calibration.mWindowInputFov.y,
    calibration.mWindowInputFovOrigin0To1.x,
    calibration.mWindowInputFovOrigin0To1.y);
  return calibration;
}

VirtualTouchScreenSink::Calibration
VirtualTouchScreenSink::CalibrationFromOpenXRView(const XrView& view) {
  using namespace DirectX::SimpleMath;
  DebugPrint(
    "Original FOV: {}l, {}r, {}u, {}d",
    view.fov.angleLeft,
    view.fov.angleRight,
    view.fov.angleUp,
    view.fov.angleRight);
  const auto poseQ = XrQuatToSM(view.pose.orientation);

  const auto leftQ = poseQ
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, view.fov.angleLeft);
  const auto rightQ = poseQ
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, view.fov.angleRight);
  const auto upQ
    = poseQ * Quaternion::CreateFromAxisAngle(Vector3::UnitX, view.fov.angleUp);
  const auto downQ = poseQ
    * Quaternion::CreateFromAxisAngle(Vector3::UnitX, view.fov.angleDown);

  const XrFovf fov = {
    .angleLeft = leftQ.ToEuler().y,
    .angleRight = rightQ.ToEuler().y,
    .angleUp = upQ.ToEuler().x,
    .angleDown = downQ.ToEuler().x,
  };

  DebugPrint(
    "Adjusted FOV: {}l, {}r, {}u, {}d",
    fov.angleLeft,
    fov.angleRight,
    fov.angleUp,
    fov.angleRight);

  XrVector2f windowInputFov {
    2 * std::max(std::abs(fov.angleRight), std::abs(fov.angleLeft)),
    std::abs(fov.angleUp) + std::abs(fov.angleDown)};

  XrVector2f fovOrigin0To1 {
    0.5,
    0.5,
  };

  return {windowInputFov, fovOrigin0To1};
}

std::optional<VirtualTouchScreenSink::Calibration>
VirtualTouchScreenSink::CalibrationFromConfig() {
  if (!Config::HaveSavedFOV) {
    return {};
  }

  const XrView view {
    .type = XR_TYPE_VIEW,
    .pose = XR_POSEF_IDENTITY,
    .fov = XrFovf {
      .angleLeft = Config::LeftEyeFOVLeft,
      .angleRight = Config::LeftEyeFOVRight,
      .angleUp = Config::LeftEyeFOVUp,
      .angleDown = Config::LeftEyeFOVDown,
  },
  };
  return CalibrationFromOpenXRView(view);
}

void VirtualTouchScreenSink::UpdateMainWindow() {
  mConsoleWindow = GetConsoleWindow();
  EnumWindows(
    &VirtualTouchScreenSink::EnumWindowCallback,
    reinterpret_cast<LPARAM>(this));
  mLastWindowCheck = std::chrono::steady_clock::now();
}

BOOL CALLBACK
VirtualTouchScreenSink::EnumWindowCallback(HWND hwnd, LPARAM lparam) {
  auto self = reinterpret_cast<VirtualTouchScreenSink*>(lparam);

  if (hwnd == self->mConsoleWindow) {
    return TRUE;
  }

  if (hwnd == self->mWindow) {
    return FALSE;
  }

  DWORD processID {};
  GetWindowThreadProcessId(hwnd, &processID);
  if (processID != self->mTargetProcessID) {
    return TRUE;
  }

  // Has a parent window
  if (GetWindow(hwnd, GW_OWNER) != (HWND)0) {
    return TRUE;
  }

  self->mWindow = hwnd;

  RECT rect {};
  GetWindowRect(hwnd, &rect);
  self->mWindowRect = rect;
  self->mWindowSize = {
    static_cast<float>(rect.right - rect.left),
    static_cast<float>(rect.bottom - rect.top),
  };
  DebugPrint(
    "Found game window; mapping hand-tracking within headset FOV to "
    "on-screen "
    "rect ({}, {}) -> ({}, {})",
    rect.left,
    rect.top,
    rect.right,
    rect.bottom);

  auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorInfo {sizeof(MONITORINFO)};
  GetMonitorInfo(monitor, &monitorInfo);

  rect = monitorInfo.rcMonitor;
  self->mScreenSize = {
    static_cast<float>(rect.right - rect.left),
    static_cast<float>(rect.bottom - rect.top),
  };

  return FALSE;
}

template <class Actual, class Wanted>
static constexpr bool HasFlags(Actual actual, Wanted wanted) {
  return (actual & wanted) == wanted;
}

bool VirtualTouchScreenSink::IsPointerSink() {
  return Config::PointerSink == PointerSink::VirtualTouchScreen;
}

static bool IsActionSink(ActionSink actionSink) {
  return (actionSink == ActionSink::VirtualTouchScreen)
    || ((actionSink == ActionSink::MatchPointerSink)
        && VirtualTouchScreenSink::IsPointerSink());
}

static bool IsClickActionSink() {
  return IsActionSink(Config::ClickActionSink);
}

static bool IsScrollActionSink() {
  return IsActionSink(Config::ScrollActionSink);
}

bool VirtualTouchScreenSink::IsActionSink() {
  return IsClickActionSink() || IsScrollActionSink();
}

bool VirtualTouchScreenSink::RotationToCartesian(
  const XrVector2f& rotation,
  XrVector2f* cartesian) {
  // Flipped because screen X is left-to-right, which is a rotation around the Y
  // axis

  const auto screenX = mCalibration.mWindowInputFovOrigin0To1.x
    + (rotation.y / mCalibration.mWindowInputFov.x);
  // OpenXR has Y origin in bottom left, screeen has it in top left
  const auto screenY = mCalibration.mWindowInputFovOrigin0To1.y
    - (rotation.x / mCalibration.mWindowInputFov.y);

  if (screenX < 0 || screenX > 1 || screenY < 0 || screenY > 1) {
    return false;
  }

  *cartesian = {screenX, screenY};

  return true;
}

void VirtualTouchScreenSink::Update(
  const InputState& left,
  const InputState& right) {
  if (right.mActions.Any()) {
    Update(right);
    return;
  }
  if (left.mActions.Any()) {
    Update(left);
    return;
  }

  if (left.mDirection && !right.mDirection) {
    Update(left);
    return;
  }

  if (right.mDirection && !left.mDirection) {
    Update(right);
    return;
  }
}

void VirtualTouchScreenSink::Update(const InputState& hand) {
  std::vector<INPUT> events;

  const auto now = std::chrono::steady_clock::now();
  const auto& rotation = hand.mDirection;
  XrVector2f xy {};
  if (IsPointerSink() && rotation && RotationToCartesian(*rotation, &xy)) {
    if (now - mLastWindowCheck > std::chrono::seconds(1)) {
      UpdateMainWindow();
    }

    const auto x = ((xy.x * mWindowSize.x) + mWindowRect.left) / mScreenSize.x;
    const auto y = ((xy.y * mWindowSize.y) + mWindowRect.top) / mScreenSize.y;

    if (Config::VerboseDebug >= 3) {
      DebugPrint(
        "Raw: ({:.02f}, {:0.2f}); adjusted for window: ({:.02f}, {:.02f})",
        xy.x,
        xy.y,
        x,
        y);
    }

    events.push_back({
      .type = INPUT_MOUSE,
      .mi = {
        .dx = std::lround(x * 65535),
        .dy = std::lround(y * 65535),
        .dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
      },
    });
  }

  if (IsClickActionSink()) {
    const auto leftClick = hand.mActions.mPrimary;
    if (leftClick != mLeftClick) {
      mLeftClick = leftClick;
      events.push_back(
        {.type = INPUT_MOUSE,
         .mi = {
           .dwFlags = static_cast<DWORD>(
             leftClick ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP),
         }});
    }

    const auto rightClick = hand.mActions.mSecondary;
    if (rightClick != mRightClick) {
      mRightClick = rightClick;
      events.push_back(
        {.type = INPUT_MOUSE,
         .mi = {
           .dwFlags = static_cast<DWORD>(
             rightClick ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP),
         }});
    }
  }

  if (IsScrollActionSink()) {
    using ValueChange = ActionState::ValueChange;
    bool isFirstScrollEvent = false;
    bool hadScrollEvent = false;
    if (hand.mActions.mValueChange != mScrollDirection) {
      mScrollDirection = hand.mActions.mValueChange;
      if (hand.mActions.mValueChange != ValueChange::None) {
        isFirstScrollEvent = true;
        mNextScrollEvent = now;
      }
    }

    if (
      hand.mActions.mValueChange == ValueChange::Decrease
      && now >= mNextScrollEvent) {
      hadScrollEvent = true;
      events.push_back({
      .type = INPUT_MOUSE,
      .mi = {
        .mouseData = static_cast<DWORD>(-WHEEL_DELTA),
        .dwFlags = MOUSEEVENTF_WHEEL,
      },
    });
    }

    if (
      hand.mActions.mValueChange == ValueChange::Increase
      && now >= mNextScrollEvent) {
      hadScrollEvent = true;
      events.push_back({
      .type = INPUT_MOUSE,
      .mi = {
        .mouseData = static_cast<DWORD>(WHEEL_DELTA),
        .dwFlags = MOUSEEVENTF_WHEEL,
      },
    });
    }

    if (hadScrollEvent) {
      if (isFirstScrollEvent) {
        mNextScrollEvent = now
          + std::chrono::milliseconds(Config::ScrollWheelDelayMilliseconds);
      } else {
        mNextScrollEvent
          += std::chrono::milliseconds(Config::ScrollWheelIntervalMilliseconds);
      }
    }
  }

  if (!events.empty()) {
    SendInput(events.size(), events.data(), sizeof(INPUT));
  }
}

}// namespace HandTrackedCockpitClicking
