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

#include <directxtk/SimpleMath.h>

#include <numbers>

using namespace DirectX::SimpleMath;

namespace DCSQuestHandTracking {

static constexpr std::string_view gInteractionProfilePath {
  "/interaction_profiles/oculus/touch_controller"};
static constexpr std::string_view gLeftHandPath {"/user/hand/left"};
static constexpr std::string_view gRightHandPath {"/user/hand/right"};
static constexpr std::string_view gAimPosePath {"/input/aim/pose"};
static constexpr std::string_view gGripPosePath {"/input/grip/pose"};
static constexpr std::string_view gSqueezeValuePath {"/input/squeeze/value"};
static constexpr std::string_view gThumbstickTouchPath {
  "/input/thumbstick/touch"};

VirtualControllerSink::VirtualControllerSink(
  const std::shared_ptr<OpenXRNext>& openXR,
  XrInstance instance,
  XrSession session,
  XrSpace viewSpace)
  : mOpenXR(openXR),
    mInstance(instance),
    mSession(session),
    mViewSpace(viewSpace) {
}

void VirtualControllerSink::Update(
  const std::optional<XrPosef>& leftAimPose,
  const std::optional<XrPosef>& rightAimPose,
  const ActionState& actionState) {
  mLeftHand.present = leftAimPose.has_value();
  if (mLeftHand.present) {
    mLeftHand.aimPose = *leftAimPose;
  }

  mRightHand.present = rightAimPose.has_value();
  if (mRightHand.present) {
    mRightHand.aimPose = *rightAimPose;
  }
}

XrResult VirtualControllerSink::xrSyncActions(
  XrSession session,
  const XrActionsSyncInfo* syncInfo) {
  for (auto hand: {&mLeftHand, &mRightHand}) {
    const bool presenceChanged = hand->present != hand->presentLastSync;
    hand->presentLastSync = hand->present;

    hand->squeezeValue.currentState = (hand->present ? 1.0f : 0.0f);
    hand->squeezeValue.isActive = hand->present;
    hand->squeezeValue.changedSinceLastSync = presenceChanged;

    hand->thumbstickTouch.currentState = (hand->present ? XR_TRUE : XR_FALSE);
    hand->thumbstickTouch.isActive = hand->present;
    hand->thumbstickTouch.changedSinceLastSync = presenceChanged;
  }

  return mOpenXR->xrSyncActions(session, syncInfo);
}

XrResult VirtualControllerSink::xrPollEvent(
  XrInstance instance,
  XrEventDataBuffer* eventData) {
  if (
    mHaveSuggestedBindings && (
    mLeftHand.present != mLeftHand.presentLastPollEvent
    || mRightHand.present != mRightHand.presentLastPollEvent)) {
    mLeftHand.presentLastPollEvent = mLeftHand.present;
    mRightHand.presentLastPollEvent = mRightHand.present;
    *reinterpret_cast<XrEventDataInteractionProfileChanged*>(eventData) = {
      .type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED,
      .session = mSession,
    };
    return XR_SUCCESS;
  }

  return mOpenXR->xrPollEvent(instance, eventData);
}

XrResult VirtualControllerSink::xrGetCurrentInteractionProfile(
  XrSession session,
  XrPath topLevelUserPath,
  XrInteractionProfileState* interactionProfile) {
  if (!mHaveSuggestedBindings) {
    return mOpenXR->xrGetCurrentInteractionProfile(
      session, topLevelUserPath, interactionProfile);
  }
  char pathBuf[XR_MAX_PATH_LENGTH];
  uint32_t pathLen;
  mOpenXR->xrPathToString(
    mInstance, topLevelUserPath, sizeof(pathBuf), &pathLen, pathBuf);
  std::string_view path {pathBuf, pathLen - 1};
  DebugPrint("Requested interaction profile for {}", path);

  if (
    (path == gLeftHandPath && mLeftHand.present)
    || (path == gRightHandPath && mRightHand.present)) {
    interactionProfile->interactionProfile = mProfilePath;
    return XR_SUCCESS;
  }

  return mOpenXR->xrGetCurrentInteractionProfile(
    session, topLevelUserPath, interactionProfile);
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
    mProfilePath = suggestedBindings->interactionProfile;
  }

  for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; ++i) {
    const auto& it = suggestedBindings->suggestedBindings[i];
    mOpenXR->xrPathToString(
      instance, it.binding, sizeof(pathBuf), &pathLen, pathBuf);
    const std::string binding {pathBuf, pathLen - 1};
    mActionPaths[it.action] = binding;

    ControllerState* state;
    if (binding.starts_with(gLeftHandPath)) {
      state = &mLeftHand;
    } else if (binding.starts_with(gRightHandPath)) {
      state = &mRightHand;
    } else {
      continue;
    }

    if (binding.ends_with(gAimPosePath)) {
      state->aimAction = it.action;
      continue;
    }

    if (binding.ends_with(gGripPosePath)) {
      state->gripAction = it.action;
      continue;
    }

    if (binding.ends_with(gSqueezeValuePath)) {
      state->squeezeValueActions.insert(it.action);
      continue;
    }

    if (binding.ends_with(gThumbstickTouchPath)) {
      state->thumbstickTouchActions.insert(it.action);
      continue;
    }
  }
  mHaveSuggestedBindings = true;

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

  mActionSpaces[*space] = createInfo->action;

  for (auto hand: {&mLeftHand, &mRightHand}) {
    if (createInfo->action == hand->aimAction) {
      hand->aimSpace = *space;
      DebugPrint(
        "Found aim space: {:#016x}", reinterpret_cast<uintptr_t>(*space));
      return XR_SUCCESS;
    }

    if (createInfo->action == hand->gripAction) {
      DebugPrint(
        "Found grip space: {:#016x}", reinterpret_cast<uintptr_t>(*space));
      hand->gripSpace = *space;
      return XR_SUCCESS;
    }
  }

  return XR_SUCCESS;
}

XrResult VirtualControllerSink::xrGetActionStateBoolean(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateBoolean* state) {
  const auto action = getInfo->action;

  for (auto hand: {&mLeftHand, &mRightHand}) {
    if (hand->thumbstickTouchActions.contains(action)) {
      *state = hand->thumbstickTouch;
      return XR_SUCCESS;
    }
  }

  return mOpenXR->xrGetActionStateBoolean(session, getInfo, state);
}

XrResult VirtualControllerSink::xrGetActionStateFloat(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateFloat* state) {
  const auto action = getInfo->action;

  for (auto hand: {&mLeftHand, &mRightHand}) {
    if (hand->squeezeValueActions.contains(action)) {
      *state = hand->squeezeValue;
      return XR_SUCCESS;
    }
  }

  return mOpenXR->xrGetActionStateFloat(session, getInfo, state);
}

static XrPosef operator*(const XrPosef& a, const XrPosef& b) {
  const Quaternion ao {
    a.orientation.x,
    a.orientation.y,
    a.orientation.z,
    a.orientation.w,
  };
  const Quaternion bo {
    b.orientation.x,
    b.orientation.y,
    b.orientation.z,
    b.orientation.w,
  };
  const Vector3 ap {
    a.position.x,
    a.position.y,
    a.position.z,
  };
  const Vector3 bp {
    b.position.x,
    b.position.y,
    b.position.z,
  };

  auto o = ao * bo;
  o.Normalize();
  const auto p = Vector3::Transform(ap, bo) + bp;

  return {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };
}

XrResult VirtualControllerSink::xrLocateSpace(
  XrSpace space,
  XrSpace baseSpace,
  XrTime time,
  XrSpaceLocation* location) {
  for (const ControllerState& hand: {mLeftHand, mRightHand}) {
    if (space != hand.aimSpace && space != hand.gripSpace) {
      continue;
    }

    if (!hand.present) {
      break;
    }

    mOpenXR->xrLocateSpace(mViewSpace, baseSpace, time, location);

    const auto viewPose = location->pose;

    // Just experimentation
    auto aimToGripQ = Quaternion::CreateFromAxisAngle(
                        Vector3::UnitX, std::numbers::pi_v<float> / 6)
      * Quaternion::CreateFromAxisAngle(
                        Vector3::UnitY, -std::numbers::pi_v<float> / 8)
      * Quaternion::CreateFromAxisAngle(
                        Vector3::UnitZ, std::numbers::pi_v<float> / 2);

    XrPosef aimToGrip {
      .orientation = {aimToGripQ.x, aimToGripQ.y, aimToGripQ.z, aimToGripQ.w},
    };

    const auto handPose = aimToGrip * hand.aimPose;

    location->pose = handPose * viewPose;

    return XR_SUCCESS;
  }

  return mOpenXR->xrLocateSpace(space, baseSpace, time, location);
}

}// namespace DCSQuestHandTracking
