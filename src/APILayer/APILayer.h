// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <openxr/openxr.h>

#include <memory>
#include <unordered_set>

#include "FrameInfo.h"
#include "InputState.h"

namespace HandTrackedCockpitClicking {

class HandTrackingSource;
class OpenXRNext;
class PointCtrlSource;
class VirtualControllerSink;
class VirtualTouchScreenSink;
struct FrameInfo;

class APILayer final {
 public:
  APILayer() = delete;
  APILayer(XrInstance, const std::shared_ptr<OpenXRNext>&);
  virtual ~APILayer();

XrResult xrGetSystemProperties(
  XrInstance instance,
  XrSystemId systemId,
  XrSystemProperties* properties);

  XrResult xrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session);

  XrResult xrBeginSession(
    XrSession session,
    const XrSessionBeginInfo* beginInfo);

  XrResult xrDestroySession(XrSession session);

  XrResult xrCreateHandTrackerEXT(
    XrSession session,
    const XrHandTrackerCreateInfoEXT* createInfo,
    XrHandTrackerEXT* handTracker);

  XrResult xrWaitFrame(
    XrSession session,
    const XrFrameWaitInfo* frameWaitInfo,
    XrFrameState* state);

  XrResult xrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings);

  XrResult xrGetCurrentInteractionProfile(
    XrSession session,
    XrPath topLevelUserPath,
    XrInteractionProfileState* interactionProfile);

  XrResult xrCreateAction(
    XrActionSet actionSet,
    const XrActionCreateInfo* createInfo,
    XrAction* action);

  XrResult xrCreateActionSpace(
    XrSession session,
    const XrActionSpaceCreateInfo* createInfo,
    XrSpace* space);

  XrResult xrGetActionStateBoolean(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateBoolean* state);

  XrResult xrGetActionStateFloat(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateFloat* state);

  XrResult xrGetActionStatePose(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStatePose* state);

  XrResult xrLocateSpace(
    XrSpace space,
    XrSpace baseSpace,
    XrTime time,
    XrSpaceLocation* location);

  XrResult xrAttachSessionActionSets(
    XrSession,
    const XrSessionActionSetsAttachInfo*);
  XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo);

  XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData);

 private:
  std::optional<XrPosef> ProjectDirection(
    const FrameInfo& frameInfo,
    const InputState& hand) const;

  struct InputSnapshot {
    FrameInfo mFrameInfo {};
    InputState mInputState {};
  };

  InputState SmoothHand(
    const InputSnapshot& currentFrame,
    const std::optional<InputSnapshot>& previousFrame) const;
  XrPosef SmoothPose(const XrPosef& current, const XrPosef& previous) const;

  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};

  std::optional<XrViewConfigurationType> mPrimaryViewConfigurationType;

  std::unordered_map<XrActionSet, std::unordered_set<XrAction>>
    mActionSetActions;
  std::unordered_set<XrAction> mAttachedActions;

  std::unique_ptr<HandTrackingSource> mHandTracking;
  std::unique_ptr<PointCtrlSource> mPointCtrl;
  std::unique_ptr<VirtualTouchScreenSink> mVirtualTouchScreen;
  std::unique_ptr<VirtualControllerSink> mVirtualController;

  std::optional<InputSnapshot> mPreviousFrameLeftHand;
  std::optional<InputSnapshot> mPreviousFrameRightHand;
};

}// namespace HandTrackedCockpitClicking
