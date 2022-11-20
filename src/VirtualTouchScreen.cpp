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

  DebugPrint(
    "Headset FOV: {} {} {} {}",
    mFov.angleLeft,
    mFov.angleRight,
    mFov.angleUp,
    mFov.angleDown);

  mTanFovX = std::tanf(mFov.angleRight);
  mTanFovY
    = std::tanf(std::max(std::abs(mFov.angleUp), std::abs(mFov.angleDown)));
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

  const auto screenY
    = hand.aimPose.position.y / (hand.aimPose.position.z * mTanFovY);
  if (screenY > 1 || screenY < -1) {
    return false;
  }

  *xy = {0.5f + (screenX / -2), 0.5f + (screenY / 2)};

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

  const auto xy = leftValid ? leftXY : rightXY;

  INPUT input {
    .type = INPUT_MOUSE,
    .mi = MOUSEINPUT {
      .dx = std::lround(xy.x * 65535),
      .dy = std::lround(xy.y * 65535),
      .dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
    },
  };

  SendInput(1, &input, sizeof(input));
}

}// namespace DCSQuestHandTracking
