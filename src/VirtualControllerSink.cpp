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

#include "VirtualControllerSink.h"

namespace DCSQuestHandTracking {

static constexpr std::string_view gInteractionProfilePath {
  "/interaction_profiles/oculus/touch_controller"};
static constexpr std::string_view gLeftHandPath {"/user/hand/left"};
static constexpr std::string_view gRightHandPath {"/user/hand/right"};
static constexpr std::string_view gAimPosePath {"/input/aim/pose"};
static constexpr std::string_view gSqueezeValuePath {"/input/squeeze/value"};

VirtualControllerSink::VirtualControllerSink(
  const std::shared_ptr<OpenXRNext>& openXR)
  : mOpenXR(openXR) {
}

void VirtualControllerSink::Update(
  const std::optional<XrPosef>& leftAimPose,
  const std::optional<XrPosef>& rightAimPose,
  const ActionState& actionState) {
}

XrResult VirtualControllerSink::xrSuggestInteractionProfileBindings(
  XrInstance instance,
  const XrInteractionProfileSuggestedBinding* suggestedBindings) {
  char pathBuf[XR_MAX_PATH_LENGTH];
  uint32_t pathLen = sizeof(pathBuf);
  mOpenXR->xrPathToString(
    instance,
    suggestedBindings->interactionProfile,
    sizeof(pathBuf),
    &pathLen,
    pathBuf);

  {
    std::string_view interactionProfile {pathBuf, pathLen - 1};

    if (interactionProfile != gInteractionProfilePath) {
      DebugPrint(
        "Profile '{}' does not match desired profile '{}'",
        interactionProfile,
        gInteractionProfilePath);
      return mOpenXR->xrSuggestInteractionProfileBindings(
        instance, suggestedBindings);
    }
    DebugPrint("Found desired profile '{}'", gInteractionProfilePath);
  }

  const bool DebugBindings = Config::VerboseDebug >= 1;

  for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; ++i) {
    const auto& it = suggestedBindings->suggestedBindings[i];
    mOpenXR->xrPathToString(
      instance, it.binding, sizeof(pathBuf), &pathLen, pathBuf);
    const std::string binding {pathBuf, pathLen - 1};
    mActionPaths[it.action] = binding;

    ControllerState* state;
    if (binding.starts_with(gLeftHandPath)) {
      state = &mLeftHand;
      if (DebugBindings) {
        DebugPrint("Binding '{}' is on left hand", binding);
      }
    } else if (binding.starts_with(gRightHandPath)) {
      state = &mRightHand;
      if (DebugBindings) {
        DebugPrint("Binding '{}' is on right hand", binding);
      }
    } else {
      DebugPrint("Binding '{}' is not a hand binding", binding);
      continue;
    }

    if (binding.ends_with(gAimPosePath)) {
      if (DebugBindings) {
        DebugPrint("Binding '{}' is an aim action", binding);
      }
      state->aimAction = it.action;
      continue;
    }

    if (binding.ends_with(gSqueezeValuePath)) {
      if (DebugBindings) {
        DebugPrint(
          "Binding '{}' is a squeeze value ({:#016x})",
          binding,
          reinterpret_cast<uintptr_t>(it.action));
      }
      state->squeezeValue.insert(it.action);
      continue;
    }

    if (DebugBindings) {
      DebugPrint("Binding '{}' is not handled by this layer", binding);
    }
  }

  return mOpenXR->xrSuggestInteractionProfileBindings(
    instance, suggestedBindings);
}

XrResult VirtualControllerSink::xrCreateActionSpace(
  XrSession session,
  const XrActionSpaceCreateInfo* createInfo,
  XrSpace* space) {
  const auto nextResult
    = mOpenXR->xrCreateActionSpace(session, createInfo, space);
  if (nextResult != XR_SUCCESS) {
    return nextResult;
  }

  if (createInfo->action == mLeftHand.aimAction) {
    DebugPrint("Found left hand aim space");
    mLeftHand.aimSpace = *space;
    return XR_SUCCESS;
  }

  if (createInfo->action == mRightHand.aimAction) {
    DebugPrint("Found right hand aim space");
    mRightHand.aimSpace = *space;
    return XR_SUCCESS;
  }

  return XR_SUCCESS;
}

XrResult VirtualControllerSink::xrGetActionStateFloat(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateFloat* state) {
  const auto action = getInfo->action;
  if (
    (mLeftHand.present && mLeftHand.squeezeValue.contains(action))
    || (mRightHand.present && mRightHand.squeezeValue.contains(action))) {
    *state = {
      .type = XR_TYPE_ACTION_STATE_FLOAT,
      .currentState = 1.0f,
      .changedSinceLastSync = XR_FALSE,
      .isActive = XR_TRUE,
    };
    return XR_SUCCESS;
  }

  return mOpenXR->xrGetActionStateFloat(session, getInfo, state);
}

}// namespace DCSQuestHandTracking
