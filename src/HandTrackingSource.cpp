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
#include "HandTrackingSource.h"

#include <directxtk/SimpleMath.h>

#include <cmath>

#include "Config.h"
#include "Environment.h"

using namespace DirectX::SimpleMath;

namespace HandTrackedCockpitClicking {

HandTrackingSource::HandTrackingSource(
  const std::shared_ptr<OpenXRNext>& next,
  XrInstance instance,
  XrSession session,
  XrSpace viewSpace,
  XrSpace localSpace)
  : mOpenXR(next),
    mInstance(instance),
    mSession(session),
    mViewSpace(viewSpace),
    mLocalSpace(localSpace) {
  DebugPrint(
    "HandTrackingSource - PointerSource: {}; PinchToClick: {}; PinchToScroll: "
    "{}",
    Config::PointerSource == PointerSource::OculusHandTracking,
    Config::PinchToClick,
    Config::PinchToScroll);
}

HandTrackingSource::~HandTrackingSource() {
  for (const auto& hand: {mLeftHand, mRightHand}) {
    if (hand.mTracker) {
      mOpenXR->xrDestroyHandTrackerEXT(hand.mTracker);
    }
  }
}

template <class Actual, class Wanted>
static constexpr bool HasFlags(Actual actual, Wanted wanted) {
  return (actual & wanted) == wanted;
}

static std::tuple<XrPosef, XrVector2f> RaycastPose(const XrPosef& pose) {
  const auto& p = pose.position;
  const auto rx = std::atan2f(p.y, -p.z);
  const auto ry = std::atan2f(p.x, -p.z);

  const auto o = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);
  return {
    {
      {o.x, o.y, o.z, o.w},
      pose.position,
    },
    {rx, ry},
  };
}

static void PopulateInteractions(
  XrHandTrackingAimFlagsFB status,
  InputState* hand) {
  hand->mPrimaryInteraction = Config::PinchToClick
    && HasFlags(status, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB);
  hand->mSecondaryInteraction = Config::PinchToClick
    && HasFlags(status, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB);
  if (!Config::PinchToScroll) {
    return;
  }

  if (HasFlags(status, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB)) {
    hand->mValueChange = InputState::ValueChange::Decrease;
    return;
  }
  if (HasFlags(status, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB)) {
    hand->mValueChange = InputState::ValueChange::Increase;
    return;
  }
}

static bool UseHandTrackingAimPointFB() {
  return Config::UseHandTrackingAimPointFB
    && Environment::Have_XR_FB_HandTracking_Aim;
}

std::tuple<InputState, InputState>
HandTrackingSource::Update(PointerMode, XrTime now, XrTime displayTime) {
  this->UpdateHand(now, displayTime, &mLeftHand);
  this->UpdateHand(now, displayTime, &mRightHand);

  const auto& leftState = mLeftHand.mState;
  const auto& rightState = mRightHand.mState;
  if (!Config::OneHandOnly) {
    return {leftState, rightState};
  }

  if (!(leftState.mPose && rightState.mPose)) {
    return {leftState, rightState};
  }

  const auto leftActive = leftState.AnyInteraction();
  const auto rightActive = rightState.AnyInteraction();
  if (leftActive && !rightActive) {
    return {leftState, {XR_HAND_RIGHT_EXT}};
  }
  if (rightActive && !leftActive) {
    return {{XR_HAND_LEFT_EXT}, rightState};
  }

  const auto lrx = leftState.mDirection->x;
  const auto lry = leftState.mDirection->y;
  const auto ldiff = (lrx * lrx) + (lry * lry);

  const auto rrx = rightState.mDirection->x;
  const auto rry = rightState.mDirection->y;
  const auto rdiff = (rrx * rrx) + (rry * rry);
  if (ldiff < rdiff) {
    return {leftState, {XR_HAND_RIGHT_EXT}};
  }
  return {{XR_HAND_LEFT_EXT}, rightState};
}

void HandTrackingSource::UpdateHand(
  XrTime now,
  XrTime displayTime,
  Hand* hand) {
  InitHandTracker(hand);
  auto& state = hand->mState;

  XrHandJointsLocateInfoEXT locateInfo {
    .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
    .baseSpace = mLocalSpace,
    .time = displayTime,
  };

  XrHandTrackingAimStateFB aimFB {XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
  std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> jointData;
  XrHandJointLocationsEXT joints {
    .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
    .jointCount = jointData.size(),
    .jointLocations = jointData.data(),
  };
  if (Environment::Have_XR_FB_HandTracking_Aim) {
    joints.next = &aimFB;
  }

  jointData.fill({});
  if (!mOpenXR->check_xrLocateHandJointsEXT(
        hand->mTracker, &locateInfo, &joints)) {
    state = {hand->mHand};
    return;
  }

  if (UseHandTrackingAimPointFB()) {
    if (HasFlags(aimFB.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
      state = {
        .mHand = hand->mHand,
        .mUpdatedAt = displayTime,
        .mPose = {aimFB.aimPose},
      };
    }
  } else if (joints.isActive) {
    const auto joint = jointData[Config::HandTrackingAimJoint];
    if (
      HasFlags(joint.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
      && HasFlags(joint.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
      state = {
        .mHand = hand->mHand,
        .mUpdatedAt = displayTime,
        .mPose = {joint.pose},
      };
    }
  }

  const auto age = std::chrono::nanoseconds(now - state.mUpdatedAt);
  const auto stale = age > std::chrono::milliseconds(200);

  if (stale) {
    state = {hand->mHand};
    return;
  }
  PopulateInteractions(aimFB.status, &state);
  const auto [raycastPose, direction] = RaycastPose(*state.mPose);
  state.mDirection = {direction};
  if (Config::HandTrackingOrientation == HandTrackingOrientation::RayCast) {
    state.mPose = raycastPose;
  }
}

void HandTrackingSource::InitHandTracker(Hand* hand) {
  if (hand->mTracker) [[likely]] {
    return;
  }

  XrHandTrackerCreateInfoEXT createInfo {
    .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
    .hand = hand->mHand,
    .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
  };
  if (!mOpenXR->check_xrCreateHandTrackerEXT(
        mSession, &createInfo, &hand->mTracker)) {
    DebugPrint(
      "Failed to initialize hand tracker for hand {}",
      static_cast<int>(hand->mHand));
    return;
  }

  DebugPrint("Initialized hand tracker {}.", static_cast<int>(hand->mHand));
}

}// namespace HandTrackedCockpitClicking
