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

#include "ActionState.h"
#include "Config.h"
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
    const std::optional<XrPosef>& leftAimPose,
    const std::optional<XrPosef>& rightAimPose,
    const ActionState& actionState);

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
  // based on /interaction_profiles/oculus/touch_controller
  struct ControllerState {
    XrHandEXT hand;

    bool present {false};
    bool presentLastSync {false};
    bool presentLastPollEvent {false};

    XrPosef aimPose {};
    XrSpace aimSpace {};
    std::unordered_set<XrAction> aimActions {};

    XrSpace gripSpace {};
    std::unordered_set<XrAction> gripActions {};

    XrActionStateFloat squeezeValue {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> squeezeValueActions {};

    XrActionStateBoolean thumbstickTouch {XR_TYPE_ACTION_STATE_BOOLEAN};
    std::unordered_set<XrAction> thumbstickTouchActions {};

    XrActionStateFloat thumbstickX {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> thumbstickXActions {};

    XrActionStateFloat thumbstickY {XR_TYPE_ACTION_STATE_FLOAT};
    std::unordered_set<XrAction> thumbstickYActions {};
  };

  bool mHaveSuggestedBindings {false};
  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSession mSession {};
  XrSpace mViewSpace {};

  XrPath mProfilePath {};
  ControllerState mLeftHand {XR_HAND_LEFT_EXT};
  ControllerState mRightHand {XR_HAND_RIGHT_EXT};

  // For debugging
  std::unordered_map<XrAction, std::string> mActionPaths;
  std::unordered_map<XrSpace, XrAction> mActionSpaces;
};

}// namespace HandTrackedCockpitClicking
