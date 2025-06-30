// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <openxr/openxr.h>

#include <tuple>

#include "InputSource.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class HandTrackingSource final : public InputSource {
 public:
  HandTrackingSource(
    const std::shared_ptr<OpenXRNext>& next,
    XrInstance instance,
    XrSession session,
    XrSpace viewSpace,
    XrSpace localSpace);
  ~HandTrackingSource();

  std::tuple<InputState, InputState> Update(PointerMode, const FrameInfo&)
    override;

  void KeepAlive(XrHandEXT, const FrameInfo&);

 private:
  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSession mSession {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};

  struct Hand {
    XrHandEXT mHand;
    InputState mState {mHand};
    XrHandTrackerEXT mTracker {};
    std::optional<XrResult> mTrackerError;
    bool mSleeping {true};
    XrTime mLastKeepAliveAt {};
    XrTime mWakeConditionsSince {};
    XrTime mHibernateGestureSince {};

    ActionState mRawActions {};
    XrTime mRawActionsSince {};
  };

  bool mHibernating {false};
  XrTime mLastHibernationChangeAt {};

  Hand mLeftHand {XR_HAND_LEFT_EXT};
  Hand mRightHand {XR_HAND_RIGHT_EXT};

  void InitHandTracker(Hand* hand);
  void UpdateHand(const FrameInfo&, Hand* hand);
  std::tuple<XrPosef, XrVector2f> RaycastPose(
    const FrameInfo&,
    const XrPosef& pose);

  enum class BeepEvent {
    Wake,
    Sleep,
    HibernateWake,
    HibernateSleep,
  };
  void PlayBeeps(BeepEvent) const;
};

}// namespace HandTrackedCockpitClicking
