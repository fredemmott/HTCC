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

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace HandTrackedCockpitClicking {

HandTrackingSource::HandTrackingSource(
  const std::shared_ptr<OpenXRNext>& next,
  XrSession session,
  XrSpace space)
  : mOpenXR(next), mSession(session), mSpace(space) {
  DebugPrint(
    "HandTrackingSource - PointerSource: {}; PinchToClick: {}; PinchToScroll: "
    "{}",
    Config::PointerSource == PointerSource::OculusHandTracking,
    Config::PinchToClick,
    Config::PinchToScroll);
}

HandTrackingSource::~HandTrackingSource() {
  if (mLeftHand) {
    mOpenXR->xrDestroyHandTrackerEXT(mLeftHand);
  }
  if (mRightHand) {
    mOpenXR->xrDestroyHandTrackerEXT(mRightHand);
  }
}

template <class Actual, class Wanted>
static constexpr bool HasFlags(Actual actual, Wanted wanted) {
  return (actual & wanted) == wanted;
}

static void DumpHandState(
  std::string_view name,
  const XrHandTrackingAimStateFB& state) {
  if (!HasFlags(state.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
    DebugPrint("{} hand not present.", name);
    return;
  }

  DebugPrint(
    "{} hand: I{} M{} R{} L{} @ ({}, {}, {})",
    name,
    HasFlags(state.status, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB),
    state.aimPose.position.x,
    state.aimPose.position.y,
    state.aimPose.position.z);
  DebugPrint(
    "  I{:.02f} M{:.02f} R{:.02f} L{:.02f}",
    state.pinchStrengthIndex,
    state.pinchStrengthMiddle,
    state.pinchStrengthRing,
    state.pinchStrengthLittle);
}

static void RaycastPose(XrPosef& pose) {
  const auto& p = pose.position;
  const auto rx = std::atan2f(p.y, -p.z);
  const auto ry = std::atan2f(p.x, -p.z);

  const auto o = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);
  pose.orientation = {o.x, o.y, o.z, o.w};
}

void HandTrackingSource::Update(XrTime displayTime) {
  InitHandTrackers();

  XrHandJointsLocateInfoEXT locateInfo {
    .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
    .baseSpace = mSpace,
    .time = displayTime,
  };

  XrHandTrackingAimStateFB aimState {XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
  std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> jointData;
  XrHandJointLocationsEXT joints {
    .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
    .jointCount = jointData.size(),
    .jointLocations = jointData.data(),
  };
  if (Environment::Have_XR_FB_HandTracking_Aim) {
    joints.next = &aimState;
  }

  auto nextResult
    = mOpenXR->xrLocateHandJointsEXT(mLeftHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    aimState.status = {};
  }
  const auto leftValid = joints.isActive;
  const auto leftAim = aimState;
  const auto leftJoints = jointData;

  nextResult = mOpenXR->xrLocateHandJointsEXT(mRightHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    aimState.status = {};
  }
  const auto rightValid = joints.isActive;
  const auto rightAim = aimState;
  const auto rightJoints = jointData;

  static std::chrono::steady_clock::time_point lastPrint {};
  const auto now = std::chrono::steady_clock::now();
  if (
    (Config::VerboseDebug >= 2)
    && (now - lastPrint > std::chrono::seconds(1))) {
    lastPrint = now;
    DumpHandState("Left", leftAim);
    DumpHandState("Right", rightAim);
  }

  mLeftHandPose = {};
  if (leftValid && !Config::UseHandTrackingAimPointFB) {
    const auto joint = leftJoints[Config::HandTrackingAimJoint];
    if (
      HasFlags(joint.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
      && HasFlags(joint.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
      mLeftHandPose = joint.pose;
    }
  } else if (HasFlags(leftAim.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
    mLeftHandPose = {leftAim.aimPose};
  }

  mRightHandPose = {};
  if (rightValid && !Config::UseHandTrackingAimPointFB) {
    const auto joint = rightJoints[Config::HandTrackingAimJoint];
    if (
      HasFlags(joint.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
      && HasFlags(joint.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
      mRightHandPose = joint.pose;
    }
  } else if (HasFlags(rightAim.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
    mRightHandPose = {rightAim.aimPose};
  }

  if (Config::OneHandOnly && mLeftHandPose && mRightHandPose) {
    constexpr auto actionBits = XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB
      | XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB
      | XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB
      | XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB;
    const auto leftAction = (leftAim.status & actionBits) != 0;
    const auto rightAction = (rightAim.status & actionBits) != 0;
    if (leftAction && !rightAction) {
      mRightHandPose = {};
    } else if (rightAction && !leftAction) {
      mLeftHandPose = {};
    } else {
      const auto& lp = mLeftHandPose->position;
      const auto lrx = std::atan2f(lp.y, -lp.z);
      const auto lry = std::atan2f(lp.x, -lp.z);
      const auto ldiff = (lrx * lrx) + (lry * lry);

      const auto& rp = mRightHandPose->position;
      const auto rrx = std::atan2f(rp.y, -rp.z);
      const auto rry = std::atan2f(rp.x, -rp.z);
      const auto rdiff = (rrx * rrx) + (rry * rry);

      if (ldiff > rdiff) {
        mLeftHandPose = {};
      } else {
        mRightHandPose = {};
      }
    }
  }

  switch (Config::HandTrackingOrientation) {
    case HandTrackingOrientation::Raw:
      break;
    case HandTrackingOrientation::RayCast:
      if (mLeftHandPose) {
        RaycastPose(*mLeftHandPose);
      }
      if (mRightHandPose) {
        RaycastPose(*mRightHandPose);
      }
      break;
  }

#define EITHER_HAS(flag) \
  (HasFlags(leftAim.status, flag) || HasFlags(rightAim.status, flag))

  mActionState = {
    .mLeftClick = Config::PinchToClick
      && EITHER_HAS(XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB),
    .mRightClick = Config::PinchToClick
      && EITHER_HAS(XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB),
    .mWheelUp = Config::PinchToScroll
      && EITHER_HAS(XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB),
    .mWheelDown = Config::PinchToScroll
      && EITHER_HAS(XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB),
  };

#undef EITHER_HAS
}

void HandTrackingSource::InitHandTrackers() {
  if (mLeftHand && mRightHand) [[likely]] {
    return;
  }

  XrHandTrackerCreateInfoEXT createInfo {
    .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
    .hand = XR_HAND_LEFT_EXT,
    .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
  };
  auto nextResult
    = mOpenXR->xrCreateHandTrackerEXT(mSession, &createInfo, &mLeftHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize left hand: {}", nextResult);
    return;
  }
  createInfo.hand = XR_HAND_RIGHT_EXT;
  nextResult
    = mOpenXR->xrCreateHandTrackerEXT(mSession, &createInfo, &mRightHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize right hand: {}", nextResult);
    return;
  }

  DebugPrint("Initialized hand trackers.");
}

std::tuple<std::optional<XrPosef>, std::optional<XrPosef>>
HandTrackingSource::GetPoses() const {
  return {mLeftHandPose, mRightHandPose};
}

std::optional<XrVector2f> HandTrackingSource::GetRXRY() const {
  auto [left, right] = GetPoses();
  if (!(left || right)) {
    return {};
  }

  const auto& hand = left ? left : right;
  const auto& pos = hand->position;

  const auto rx = std::atan2f(pos.y, -pos.z);
  const auto ry = std::atan2f(pos.x, -pos.z);
  return {{rx, ry}};
}

ActionState HandTrackingSource::GetActionState() const {
  return mActionState;
}

}// namespace HandTrackedCockpitClicking
