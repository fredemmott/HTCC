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
#pragma once

#include <openxr/openxr.h>

#include <unordered_map>
#include <unordered_set>

#include "Config.h"
#include "FrameInfo.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

struct InputState;

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
    bool haveAction = false;

    bool present {false};
    bool presentLastSync {false};
    bool presentLastPollEvent {false};

    XrPosef savedAimPose {};
    XrPosef aimPose {};
    XrSpace aimSpace {};
    std::unordered_set<XrAction> aimActions {};

    XrSpace gripSpace {};
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

    // MSFS

    XrActionStateBoolean triggerValue {XR_TYPE_ACTION_STATE_BOOLEAN};
    std::unordered_set<XrAction> triggerValueActions {};

    Rotation mRotationDirection {Rotation::None};
    float mRotationAngle {0};
    XrTime mLastRotatedAt {};

    XrTime mBlockSecondaryActionsUntil {};
  };

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

  void SetControllerActions(
    XrTime predictedDisplayTime,
    const InputState& hand,
    ControllerState* controller);
  void SetDCSControllerActions(
    const InputState& hand,
    ControllerState* controller);
  void SetMSFSControllerActions(
    XrTime predictedDisplayTime,
    const InputState& hand,
    ControllerState* controller);

  void UpdateHand(
    const FrameInfo& frameInfo,
    const InputState& hand,
    ControllerState* controller);

  // For debugging
  std::unordered_map<XrAction, std::string> mActionPaths;
  std::unordered_map<XrSpace, XrAction> mActionSpaces;
};

}// namespace HandTrackedCockpitClicking
