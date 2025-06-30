// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
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
    Config::PointerSource == PointerSource::OpenXRHandTracking,
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

std::tuple<XrPosef, XrVector2f> HandTrackingSource::RaycastPose(
  const FrameInfo& frameInfo,
  const XrPosef& pose) {
  const auto& p = (pose * frameInfo.mLocalInView).position;
  const auto rx = std::atan2f(p.y, -p.z);
  const auto ry = std::atan2f(p.x, -p.z);

  const auto o = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);
  const XrPosef retView = {
    {o.x, o.y, o.z, o.w},
    pose.position,
  };

  return {
    {
      (retView * frameInfo.mViewInLocal).orientation,
      pose.position,
    },
    {rx, ry},
  };
}

static void PopulateInteractions(
  XrHandTrackingAimFlagsFB status,
  ActionState* hand) {
  hand->mPrimary = Config::PinchToClick
    && HasFlags(status, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB);
  hand->mSecondary = Config::PinchToClick
    && HasFlags(status, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB);
  if (!Config::PinchToScroll) {
    return;
  }

  if (HasFlags(status, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB)) {
    hand->mValueChange = ActionState::ValueChange::Decrease;
    return;
  }
  if (HasFlags(status, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB)) {
    hand->mValueChange = ActionState::ValueChange::Increase;
    return;
  }
}

static bool UseHandTrackingAimPointFB() {
  return Config::UseHandTrackingAimPointFB
    && Environment::Have_XR_FB_hand_tracking_aim;
}

std::tuple<InputState, InputState> HandTrackingSource::Update(
  PointerMode,
  const FrameInfo& frameInfo) {
  this->UpdateHand(frameInfo, &mLeftHand);
  this->UpdateHand(frameInfo, &mRightHand);

  const auto& leftState = mLeftHand.mState;
  const auto& rightState = mRightHand.mState;
  if (!Config::OneHandOnly) {
    return {leftState, rightState};
  }

  if (!(leftState.mPose && rightState.mPose)) {
    return {leftState, rightState};
  }

  if (!(leftState.mDirection && rightState.mDirection)) {
    return {leftState, rightState};
  }

  const auto leftActive = leftState.mActions.Any();
  const auto rightActive = rightState.mActions.Any();
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

void HandTrackingSource::KeepAlive(XrHandEXT handID, const FrameInfo& info) {
  auto& hand = (handID == XR_HAND_LEFT_EXT) ? mLeftHand : mRightHand;
  hand.mLastKeepAliveAt = info.mNow;
}

void HandTrackingSource::UpdateHand(const FrameInfo& frameInfo, Hand* hand) {
  InitHandTracker(hand);

  if (!hand->mTracker) {
    return;
  }

  const auto displayTime = frameInfo.mPredictedDisplayTime;

  auto& state = hand->mState;
  state.mHand = hand->mHand;

  XrHandJointsLocateInfoEXT locateInfo {
    .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
    .baseSpace = mLocalSpace,
    .time = displayTime,
  };

  std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> jointLocations;
  jointLocations.fill({});

  XrHandJointLocationsEXT joints {
    .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
    .jointCount = jointLocations.size(),
    .jointLocations = jointLocations.data(),
  };

  XrHandTrackingAimStateFB aimFB {XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
  if (Environment::Have_XR_FB_hand_tracking_aim) {
    joints.next = &aimFB;
  }

  if (!mOpenXR->check_xrLocateHandJointsEXT(
        hand->mTracker, &locateInfo, &joints)) {
    state = {hand->mHand};
    return;
  }

  if (UseHandTrackingAimPointFB()) {
    if (HasFlags(aimFB.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
      state.mPositionUpdatedAt = frameInfo.mNow;
      state.mPose = {aimFB.aimPose};
    }
  } else if (joints.isActive) {
    const auto joint = jointLocations[Config::HandTrackingAimJoint];
    if (
      HasFlags(joint.locationFlags, XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
      && HasFlags(joint.locationFlags, XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
      state.mPositionUpdatedAt = frameInfo.mNow;
      state.mPose = {joint.pose};
    }
  }

  if (!state.mPose) {
    state = {hand->mHand};
    if (
      hand->mLastKeepAliveAt && (!hand->mSleeping)
      && std::chrono::nanoseconds(frameInfo.mNow - hand->mLastKeepAliveAt)
        >= std::chrono::milliseconds(Config::HandTrackingSleepMilliseconds)) {
      hand->mWakeConditionsSince = {};
      hand->mHibernateGestureSince = {};
      hand->mSleeping = true;
      PlayBeeps(BeepEvent::Sleep);
    }
    return;
  }

  const auto age
    = std::chrono::nanoseconds(frameInfo.mNow - state.mPositionUpdatedAt);
  if (age > std::chrono::milliseconds(200)) {
    state = {hand->mHand};
    return;
  }

  const auto [raycastPose, rotation] = RaycastPose(frameInfo, *state.mPose);

  const auto arx = std::abs(rotation.x);
  const auto ary = std::abs(rotation.y);
  if (
    arx <= (Config::HandTrackingWakeVFOV / 2)
    && ary <= (Config::HandTrackingWakeHFOV / 2)) {
    if (!hand->mWakeConditionsSince) {
      hand->mWakeConditionsSince = frameInfo.mNow;
    }
  } else {
    hand->mWakeConditionsSince = {};
  }

  const bool wasSleeping = hand->mSleeping;

  const auto inActionFOV = arx <= (Config::HandTrackingActionVFOV / 2)
    && ary <= (Config::HandTrackingActionHFOV / 2);

  if (inActionFOV) {
    hand->mLastKeepAliveAt = frameInfo.mNow;
  }

  if (
    hand->mWakeConditionsSince
    && std::chrono::nanoseconds(frameInfo.mNow - hand->mWakeConditionsSince)
      >= std::chrono::milliseconds(Config::HandTrackingWakeMilliseconds)) {
    hand->mSleeping = false;
  } else if (
    hand->mLastKeepAliveAt
    && std::chrono::nanoseconds(frameInfo.mNow - hand->mLastKeepAliveAt)
      >= std::chrono::milliseconds(Config::HandTrackingSleepMilliseconds)) {
    hand->mSleeping = true;
  }

  {
    ActionState rawActions {};
    PopulateInteractions(aimFB.status, &rawActions);
    if (rawActions != hand->mRawActions) {
      hand->mRawActionsSince = frameInfo.mNow;
      hand->mRawActions = rawActions;
    } else if (
      hand->mRawActionsSince
      && std::chrono::nanoseconds(frameInfo.mNow - hand->mRawActionsSince)
        >= std::chrono::milliseconds(Config::HandTrackingGestureMilliseconds)) {
      // Inverted because l-r movement is rotation in x axis
#define FILTER_ACTION(x) \
  state.mActions.x = rawActions.x && (state.mActions.x || inActionFOV)
      FILTER_ACTION(mPrimary);
      FILTER_ACTION(mSecondary);
#undef FILTER_ACTION
      using ValueChange = ActionState::ValueChange;
      if (
        inActionFOV
        || (rawActions.mValueChange == state.mActions.mValueChange)) {
        state.mActions.mValueChange = rawActions.mValueChange;
      } else {
        state.mActions.mValueChange = ValueChange::None;
      }
    }
  }

  if (
    Config::HandTrackingHibernateGestureEnabled
    && Config::HandTrackingHibernateIntervalMilliseconds
    && Config::HandTrackingHibernateCutoff > 0.001
    && rotation.x >= Config::HandTrackingHibernateCutoff
    && hand->mState.mPose->position.y > frameInfo.mViewInLocal.position.y
    && std::chrono::nanoseconds(frameInfo.mNow - mLastHibernationChangeAt)
      >= std::chrono::milliseconds(
         Config::HandTrackingHibernateIntervalMilliseconds)) {
    if (!hand->mHibernateGestureSince) {
      hand->mHibernateGestureSince = frameInfo.mNow;
    }
  } else {
    hand->mHibernateGestureSince = {};
  }

  if (state.mActions.Any()) {
    hand->mLastKeepAliveAt = frameInfo.mNow;
    hand->mHibernateGestureSince = {};

    hand->mSleeping = false;
  }

  if (hand->mSleeping && !wasSleeping) {
    DebugPrint("Sleeping hand {}", static_cast<int>(hand->mHand));
    PlayBeeps(BeepEvent::Sleep);
  } else if (wasSleeping && !hand->mSleeping) {
    DebugPrint("Waking hand {}", static_cast<int>(hand->mHand));
    PlayBeeps(BeepEvent::Wake);
  }

  if (
    hand->mHibernateGestureSince && Config::HandTrackingHibernateMilliseconds
    && std::chrono::nanoseconds(frameInfo.mNow - hand->mHibernateGestureSince)
      >= std::chrono::milliseconds(Config::HandTrackingHibernateMilliseconds)) {
    hand->mHibernateGestureSince = {};
    mLastHibernationChangeAt = frameInfo.mNow;
    if (mHibernating) {
      DebugPrint("Waking from hibernation");
      PlayBeeps(BeepEvent::HibernateWake);
      mHibernating = false;
    } else {
      DebugPrint("Entering hibernation");
      PlayBeeps(BeepEvent::HibernateSleep);
      mHibernating = true;
    }
  }

  if (hand->mSleeping || mHibernating) {
    state = {hand->mHand};
    return;
  }

  state.mDirection = {rotation};
  switch (Config::HandTrackingOrientation) {
    case HandTrackingOrientation::Raw:
      break;
    case HandTrackingOrientation::RayCast:
      state.mPose = raycastPose;
      break;
    case HandTrackingOrientation::RayCastWithReprojection:
      // reproject from direction
      state.mPose = {};
      break;
  }
}

void HandTrackingSource::InitHandTracker(Hand* hand) {
  if (hand->mTracker) {
    return;
  }
  if (hand->mTrackerError) {
    return;
  }

  if (
    Config::HandTrackingHands == HandTrackingHands::Left
    && hand->mHand == XR_HAND_RIGHT_EXT) {
    return;
  }

  if (
    Config::HandTrackingHands == HandTrackingHands::Right
    && hand->mHand == XR_HAND_LEFT_EXT) {
    return;
  }

  XrHandTrackerCreateInfoEXT createInfo {
    .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
    .hand = hand->mHand,
    .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
  };
  const auto result = mOpenXR->xrCreateHandTrackerEXT(
        mSession, &createInfo, &hand->mTracker);
  if (!XR_SUCCEEDED(result)) {
    hand->mTrackerError = result;
    DebugPrint(
      "Failed to initialize hand tracker for hand {} - {}",
      static_cast<int>(hand->mHand), result);
    return;
  }

  DebugPrint("Initialized hand tracker {}.", static_cast<int>(hand->mHand));
}

void HandTrackingSource::PlayBeeps(BeepEvent event) const {
  switch (event) {
    case BeepEvent::Wake:
    case BeepEvent::Sleep:
      if (!Config::HandTrackingWakeSleepBeeps) {
        return;
      }
    case BeepEvent::HibernateWake:
    case BeepEvent::HibernateSleep:
      if (!Config::HandTrackingHibernateBeeps) {
        return;
      }
  }

  std::thread beepThread {[event]() {
    constexpr DWORD lowNote {262};// C4
    constexpr DWORD highNote {440};// A4
    constexpr DWORD ms = {100};

    switch (event) {
      case BeepEvent::HibernateWake:
        Beep(lowNote, ms);
        Beep(highNote, ms);
        [[fallthrough]];
      case BeepEvent::Wake:
        Beep(lowNote, ms);
        Beep(highNote, ms);
        return;
      case BeepEvent::HibernateSleep:
        Beep(highNote, ms);
        Beep(lowNote, ms);
        [[fallthrough]];
      case BeepEvent::Sleep:
        Beep(highNote, ms);
        Beep(lowNote, ms);
        return;
    }
  }};
  beepThread.detach();
}

}// namespace HandTrackedCockpitClicking
