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
#include "VirtualTouchScreen.h"

#include <cmath>

#include "Config.h"
#include "DebugPrint.h"

namespace DCSQuestHandTracking {

VirtualTouchScreen::VirtualTouchScreen(
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
  XrView views[2];
  views[0].type = XR_TYPE_VIEW;
  views[1].type = XR_TYPE_VIEW;
  uint32_t viewCount {2};
  auto nextResult = oxr->xrLocateViews(
    session, &viewLocateInfo, &viewState, viewCount, &viewCount, views);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to find FOV: {}", (int)nextResult);
    return;
  }

  mFov = views[0].fov;
  mFov.angleRight = views[1].fov.angleRight;

  mTanFovX = std::tanf(mFov.angleRight);
  mTanFovUp = std::tanf(mFov.angleUp);
  mTanFovDown = std::tanf(std::abs(mFov.angleDown));
  mNormalizedFovOriginY = std::abs(mFov.angleUp)
    / (std::abs(mFov.angleUp) + std::abs(mFov.angleDown));

  DebugPrint(
    "Headset FOV: {} {} {} {} - vertical split at {}",
    mFov.angleLeft,
    mFov.angleRight,
    mFov.angleUp,
    mFov.angleDown,
    mNormalizedFovOriginY);

  UpdateMainWindow();
}

void VirtualTouchScreen::UpdateMainWindow() {
  mThisProcess = GetCurrentProcessId();
  mConsoleWindow = GetConsoleWindow();
  EnumWindows(
    &VirtualTouchScreen::EnumWindowCallback, reinterpret_cast<LPARAM>(this));
  mLastWindowCheck = std::chrono::steady_clock::now();
}

BOOL CALLBACK VirtualTouchScreen::EnumWindowCallback(HWND hwnd, LPARAM lparam) {
  auto this_ = reinterpret_cast<VirtualTouchScreen*>(lparam);
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

bool VirtualTouchScreen::NormalizeHand(
  const XrHandTrackingAimStateFB& hand,
  XrVector2f* xy) {
  if (!HasFlags(hand.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
    return false;
  }

  // Assuming left and right FOV are identical...
  const auto screenX
    = hand.aimPose.position.x / (hand.aimPose.position.z * mTanFovX);
  if (screenX > 1 || screenX < -1) {
    return false;
  }

  const auto isUp = hand.aimPose.position.y >= 0;
  const auto tanY = isUp ? mTanFovUp : mTanFovDown;

  const auto screenY
    = hand.aimPose.position.y / (hand.aimPose.position.z * tanY);
  if (screenY > 1 || screenY < -1) {
    return false;
  }

  *xy = {0.5f + (screenX / -2), mNormalizedFovOriginY + (screenY / 2)};

  return true;
}

void VirtualTouchScreen::SubmitData(
  const XrHandTrackingAimStateFB& left,
  const XrHandTrackingAimStateFB& right) {
  XrVector2f leftXY {};
  XrVector2f rightXY {};
  const bool leftValid = NormalizeHand(left, &leftXY);
  const bool rightValid = NormalizeHand(right, &rightXY);

  if (leftValid == rightValid) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (now - mLastWindowCheck > std::chrono::seconds(1)) {
    UpdateMainWindow();
  }

  const auto& xy = leftValid ? leftXY : rightXY;

  const auto x = ((xy.x * mWindowSize.x) + mWindowRect.left) / mScreenSize.x;
  const auto y = ((xy.y * mWindowSize.y) + mWindowRect.top) / mScreenSize.y;

  if (Config::VerboseDebug) {
    DebugPrint(
      "Raw: ({:.02f}, {:0.2f}); adjusted for window: ({:.02f}, {:.02f})",
      xy.x,
      xy.y,
      x,
      y);
  }

  std::vector<INPUT> events {
    INPUT {
      .type = INPUT_MOUSE,
      .mi = {
        .dx = std::lround(x * 65535),
        .dy = std::lround(y * 65535),
        .dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
      },
    }
  };

  const auto& hand = leftValid ? left : right;
  const auto flags = hand.status;

  if (Config::PinchToClick) {
    const auto leftClick
      = HasFlags(flags, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB);
    if (leftClick != mLeftClick) {
      mLeftClick = leftClick;
      events.push_back(
        {.type = INPUT_MOUSE,
         .mi = {
           .dwFlags = static_cast<DWORD>(
             leftClick ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP),
         }});
    }

    const auto rightClick
      = HasFlags(flags, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB);
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

  if (Config::PinchToScroll) {
    const auto wheelUp
      = HasFlags(flags, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB);
    const auto wheelDown
      = HasFlags(flags, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB);
    if (
      wheelUp && (!wheelDown)
      && (now - mLastWheelUp > std::chrono::milliseconds(250))) {
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
      wheelDown && (!wheelUp)
      && (now - mLastWheelDown > std::chrono::milliseconds(250))) {
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

  SendInput(events.size(), events.data(), sizeof(INPUT));
}

}// namespace DCSQuestHandTracking
