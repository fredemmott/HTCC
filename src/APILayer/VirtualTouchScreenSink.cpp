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
  const std::shared_ptr<OpenXRNext>& oxr,
  XrSession session,
  XrTime nextDisplayTime,
  XrSpace viewSpace) {
  DebugPrint(
    "Initialized virtual touch screen - PointerSink: {}; ActionSink: {}",
    IsPointerSink(),
    IsActionSink());
  XrViewLocateInfo viewLocateInfo {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    .displayTime = nextDisplayTime,
    .space = viewSpace,
  };
  XrViewState viewState {XR_TYPE_VIEW_STATE};
  std::array<XrView, 2> views;
  views.fill({XR_TYPE_VIEW});
  uint32_t viewCount {views.size()};
  if (!oxr->check_xrLocateViews(
        session,
        &viewLocateInfo,
        &viewState,
        viewCount,
        &viewCount,
        views.data())) {
    DebugPrint("Failed to find FOV");
    return;
  }

  mFov = views[Config::MirrorEye].fov;

  mCombinedFov = {
    std::abs(mFov.angleLeft) + std::abs(mFov.angleRight),
    std::abs(mFov.angleUp) + std::abs(mFov.angleDown),
  };

  mFovOrigin0To1 = {
    std::abs(mFov.angleLeft) / mCombinedFov.x,
    std::abs(mFov.angleUp) / mCombinedFov.y,
  };

  DebugPrint(
    "Reported eye FOV: L {} R {} U {} D {} ({}x{}) - tracking origin at ({}, "
    "{})",
    mFov.angleLeft,
    mFov.angleRight,
    mFov.angleUp,
    mFov.angleDown,
    mCombinedFov.x,
    mCombinedFov.y,
    mFovOrigin0To1.x,
    mFovOrigin0To1.y);

  UpdateMainWindow();
}

void VirtualTouchScreenSink::UpdateMainWindow() {
  mThisProcess = GetCurrentProcessId();
  mConsoleWindow = GetConsoleWindow();
  EnumWindows(
    &VirtualTouchScreenSink::EnumWindowCallback,
    reinterpret_cast<LPARAM>(this));
  mLastWindowCheck = std::chrono::steady_clock::now();
}

BOOL CALLBACK
VirtualTouchScreenSink::EnumWindowCallback(HWND hwnd, LPARAM lparam) {
  auto this_ = reinterpret_cast<VirtualTouchScreenSink*>(lparam);
  DWORD processID {};
  GetWindowThreadProcessId(hwnd, &processID);
  if (processID != this_->mThisProcess) {
    return TRUE;
  }

  // Has a parent window
  if (GetWindow(hwnd, GW_OWNER) != (HWND)0) {
    return TRUE;
  }

  if (hwnd == this_->mConsoleWindow) {
    return TRUE;
  }

  RECT rect {};
  GetWindowRect(hwnd, &rect);
  this_->mWindowRect = rect;
  this_->mWindowSize = {
    static_cast<float>(rect.right - rect.left),
    static_cast<float>(rect.bottom - rect.top),
  };

  if (hwnd == this_->mWindow) {
    return FALSE;
  }

  this_->mWindow = hwnd;
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
  this_->mScreenSize = {
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

  const auto screenX = mFovOrigin0To1.x + (rotation.y / mCombinedFov.x);
  // OpenXR has Y origin in bottom left, screeen has it in top left
  const auto screenY = mFovOrigin0To1.y - (rotation.x / mCombinedFov.y);

  if (screenX < 0 || screenX > 1 || screenY < 0 || screenY > 1) {
    return false;
  }

  *cartesian = {screenX, screenY};

  return true;
}

void VirtualTouchScreenSink::Update(
  const InputState& left,
  const InputState& right) {
  if (right.AnyInteraction()) {
    Update(right);
    return;
  }
  if (left.AnyInteraction()) {
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
    const auto leftClick = hand.mPrimaryInteraction;
    if (leftClick != mLeftClick) {
      mLeftClick = leftClick;
      events.push_back(
        {.type = INPUT_MOUSE,
         .mi = {
           .dwFlags = static_cast<DWORD>(
             leftClick ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP),
         }});
    }

    const auto rightClick = hand.mSecondaryInteraction;
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
    using ValueChange = InputState::ValueChange;
    if (hand.mValueChange != mScrollDirection) {
      mScrollDirection = hand.mValueChange;
      if (hand.mValueChange == ValueChange::None) {
        mScrollStartTime = {};
      } else {
        mScrollStartTime = now;
      }
    }
    const auto multiplier = GetScrollMultiplier(now);
    if (
      hand.mValueChange == ValueChange::Decrease
      && (now - mLastWheelUp > std::chrono::milliseconds(Config::ScrollWheelMilliseconds))) {
      mLastWheelUp = now;
      events.push_back({
      .type = INPUT_MOUSE,
      .mi = {
        .mouseData = static_cast<DWORD>(-WHEEL_DELTA),
        .dwFlags = MOUSEEVENTF_WHEEL,
      },
    });
    }

    if (
      hand.mValueChange == ValueChange::Increase
      && (now - mLastWheelDown > std::chrono::milliseconds(Config::ScrollWheelMilliseconds))) {
      mLastWheelDown = now;
      events.push_back({
      .type = INPUT_MOUSE,
      .mi = {
        .mouseData = static_cast<DWORD>(WHEEL_DELTA),
        .dwFlags = MOUSEEVENTF_WHEEL,
      },
    });
    }
  }

  if (!events.empty()) {
    SendInput(events.size(), events.data(), sizeof(INPUT));
  }
}

uint8_t VirtualTouchScreenSink::GetScrollMultiplier(
  const std::chrono::steady_clock::time_point& now) const {
  using ValueChange = InputState::ValueChange;
  if (mScrollDirection == ValueChange::None) {
    return 0;
  }

  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - mScrollStartTime)
                    .count();
  return std::clamp<uint8_t>(
    1 + (ms / Config::ScrollAccelerationMilliseconds), 1, 4);
}

}// namespace HandTrackedCockpitClicking
