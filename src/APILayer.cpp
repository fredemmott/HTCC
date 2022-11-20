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

#include "APILayer.h"

#include <loader_interfaces.h>
#include <openxr/openxr.h>

#include <memory>
#include <string>
#include <vector>

#include "DebugPrint.h"
#include "OpenXRNext.h"

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace DCSQuestHandTracking {

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

APILayer::APILayer(XrSession session, const std::shared_ptr<OpenXRNext>& next)
  : mOpenXR(next) {
  DebugPrint("{}()", __FUNCTION__);
  auto oxr = next.get();

  XrReferenceSpaceCreateInfo referenceSpace {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .next = nullptr,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
    .poseInReferenceSpace = XR_POSEF_IDENTITY,
  };

  XrResult nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mLocalSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create local space: {}", nextResult);
    return;
  }

  referenceSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mViewSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create view space: {}", nextResult);
    return;
  }

  InitHandTrackers(session);
  DebugPrint("Fully initialized.");
}

void APILayer::InitHandTrackers(XrSession session) {
  if (mLeftHand && mRightHand) [[likely]] {
    return;
  }
  XrHandTrackerCreateInfoEXT createInfo {
    .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
    .hand = XR_HAND_LEFT_EXT,
    .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
  };
  auto nextResult
    = mOpenXR->xrCreateHandTrackerEXT(session, &createInfo, &mLeftHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize left hand: {}", nextResult);
    return;
  }
  createInfo.hand = XR_HAND_RIGHT_EXT;
  nextResult
    = mOpenXR->xrCreateHandTrackerEXT(session, &createInfo, &mRightHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize right hand: {}", nextResult);
    return;
  }

  DebugPrint("Initialized hand trackers.");
}

APILayer::~APILayer() {
  if (mLocalSpace) {
    mOpenXR->xrDestroySpace(mLocalSpace);
  }
  if (mViewSpace) {
    mOpenXR->xrDestroySpace(mViewSpace);
  }
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

XrResult APILayer::xrEndFrame(
  XrSession session,
  const XrFrameEndInfo* frameEndInfo) {
  InitHandTrackers(session);

  XrHandJointsLocateInfoEXT locateInfo {
    .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
    .baseSpace = mViewSpace,
    .time = frameEndInfo->displayTime,
  };

  XrHandTrackingAimStateFB aimState {XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
  XrHandJointLocationEXT jointData[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT joints {
    .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
    .next = &aimState,
    .jointCount = XR_HAND_JOINT_COUNT_EXT,
    .jointLocations = jointData,
  };

  auto nextResult
    = mOpenXR->xrLocateHandJointsEXT(mLeftHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to get left hand position: {}", nextResult);
  }
  const auto left = aimState;
  nextResult = mOpenXR->xrLocateHandJointsEXT(mRightHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to get right hand position: {}", nextResult);
  }
  const auto right = aimState;

  static std::chrono::steady_clock::time_point lastPrint {};
  const auto now = std::chrono::steady_clock::now();
  if (Config::VerboseDebug && (now - lastPrint > std::chrono::seconds(1))) {
    lastPrint = now;
    DumpHandState("Left", left);
    DumpHandState("Right", right);
  }

  if (!mVirtualTouchScreen) [[unlikely]] {
    mVirtualTouchScreen = std::make_unique<VirtualTouchScreen>(
      mOpenXR, session, frameEndInfo->displayTime, mViewSpace);
  }
  mVirtualTouchScreen->SubmitData(left, right);

  return mOpenXR->xrEndFrame(session, frameEndInfo);
}

}// namespace DCSQuestHandTracking
