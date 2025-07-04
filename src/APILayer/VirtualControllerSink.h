// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <openxr/openxr.h>

#include <unordered_map>
#include <unordered_set>

#include "Config.h"
#include "FrameInfo.h"
#include "InputState.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class VirtualControllerSink final {
 public:
  VirtualControllerSink(
    const std::shared_ptr<OpenXRNext>& oxr,
    XrInstance instance,
    XrSession session,
    XrSpace viewSpace);

  void Update(
    const FrameInfo&,
    const InputState& leftHand,
    const InputState& rightHand);

  static bool IsActionSink();
  static bool IsPointerSink();

  XrResult xrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings);

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

  XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo);

  XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData);

  XrResult xrGetCurrentInteractionProfile(
    XrSession session,
    XrPath topLevelUserPath,
    XrInteractionProfileState* interactionProfile);

 private:
  // Move the pose down, and angle upwards, so it's not blocked by the
  // controller model
  XrPosef OffsetPointerPose(const FrameInfo&, const XrPosef& original);

  enum class Rotation { None, Clockwise, CounterClockwise };
  struct ControllerState {
    XrHandEXT hand {};
    XrPath path {};

    bool present {false};
    bool presentLastSync {false};
    bool presentLastPollEvent {false};

    std::optional<XrVector2f> mPreviousFrameDirection;

    std::optional<XrPosef> savedAimPose {};
    bool mUnlockedPosition {false};
    XrPosef aimPose {};
    std::unordered_set<XrSpace> aimSpaces {};
    std::unordered_set<XrAction> aimActions {};

    std::unordered_set<XrSpace> gripSpaces {};
    std::unordered_set<XrAction> gripActions {};

    // Cosmetic and 'is using controller'
    XrActionStateFloat squeezeValue {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> squeezeValueActions {};

    // Cosmetic and DCS
    XrActionStateBoolean thumbstickTouch {XR_TYPE_ACTION_STATE_BOOLEAN};
    std::unordered_set<XrAction> thumbstickTouchActions {};

    // Cosmetic and MSFS
    XrActionStateBoolean triggerTouch {XR_TYPE_ACTION_STATE_BOOLEAN};
    std::unordered_set<XrAction> triggerTouchActions {};

    // DCS

    XrActionStateFloat thumbstickX {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> thumbstickXActions {};

    XrActionStateFloat thumbstickY {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> thumbstickYActions {};

    ActionState::ValueChange mValueChange {ActionState::ValueChange::None};
    XrTime mValueChangeStartAt {};

    // MSFS

    XrActionStateBoolean triggerValue {XR_TYPE_ACTION_STATE_BOOLEAN};
    std::unordered_set<XrAction> triggerValueActions {};

    Rotation mRotationDirection {Rotation::None};
    float mRotationAngle {0};
    XrTime mLastRotationAt {};

    XrTime mBlockSecondaryActionsUntil {};
  };

  std::optional<XrPosef> GetInputPose(
    const FrameInfo&,
    const InputState& hand,
    ControllerState* controller);

  bool mHaveSuggestedBindings {false};
  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSession mSession {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};

  XrPath mProfilePath {};
  ControllerState mLeftController {XR_HAND_LEFT_EXT};
  ControllerState mRightController {XR_HAND_RIGHT_EXT};

  std::string_view ResolvePath(XrPath path);
  std::unordered_map<XrPath, std::string> mPaths;
  std::unordered_map<XrAction, std::unordered_set<XrSpace>> mActionSpaces;

  void SetControllerActions(
    XrTime predictedDisplayTime,
    const ActionState& hand,
    ControllerState* controller);
  void SetDCSControllerActions(
    XrTime predictedDisplayTime,
    const ActionState& hand,
    ControllerState* controller);
  void SetMSFSControllerActions(
    XrTime predictedDisplayTime,
    const ActionState& hand,
    ControllerState* controller);

  void UpdateHand(
    const FrameInfo& frameInfo,
    const InputState& hand,
    ControllerState* controller);

  void AddBinding(XrPath, XrAction);
};

}// namespace HandTrackedCockpitClicking
