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
#include <openxr/openxr_platform.h>

#include <numbers>

#include "Environment.h"
#include "math.h"

using namespace DirectX::SimpleMath;

namespace HandTrackedCockpitClicking {

static constexpr std::string_view gLeftHandPath {"/user/hand/left"};
static constexpr std::string_view gRightHandPath {"/user/hand/right"};
static constexpr std::string_view gAimPosePath {"/input/aim/pose"};
static constexpr std::string_view gGripPosePath {"/input/grip/pose"};
static constexpr std::string_view gSqueezeValuePath {"/input/squeeze/value"};
static constexpr std::string_view gThumbstickTouchPath {
  "/input/thumbstick/touch"};
static constexpr std::string_view gThumbstickXPath {"/input/thumbstick/x"};
static constexpr std::string_view gThumbstickYPath {"/input/thumbstick/y"};
static constexpr std::string_view gTriggerTouchPath {"/input/trigger/touch"};
static constexpr std::string_view gTriggerValuePath {"/input/trigger/value"};

static constexpr XrVector3f gPushOffset {0.0f, 0.0f, -0.02f};

static bool UseDCSActions() {
  return VirtualControllerSink::IsActionSink()
    && (Config::VRControllerActionSinkMapping
        == VRControllerActionSinkMapping::DCS);
}

static bool UseMSFSActions() {
  return VirtualControllerSink::IsActionSink()
    && (Config::VRControllerActionSinkMapping
        == VRControllerActionSinkMapping::MSFS);
}

VirtualControllerSink::VirtualControllerSink(
  const std::shared_ptr<OpenXRNext>& openXR,
  XrInstance instance,
  XrSession session,
  XrSpace viewSpace)
  : mOpenXR(openXR),
    mInstance(instance),
    mSession(session),
    mViewSpace(viewSpace) {
  DebugPrint(
    "Initialized virtual VR controller - PointerSink: {}; ActionSink: {}",
    IsPointerSink(),
    IsActionSink());

  QueryPerformanceFrequency(&mPerformanceCounterFrequency);
}

bool VirtualControllerSink::IsPointerSink() {
  if (Config::PointerSink == PointerSink::VirtualVRController) {
    if (!Environment::Have_XR_KHR_win32_convert_performance_counter_time) {
      // This should pretty much never happen: every runtime supports this
      // extension
      DebugPrint(
        "Configured to use VirtualControllerSink, but don't have {}",
        XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME);
      return false;
    }
    return true;
  }
  return false;
}

bool VirtualControllerSink::IsActionSink() {
  return (Config::ActionSink == ActionSink::VirtualVRController)
    || ((Config::ActionSink == ActionSink::MatchPointerSink)
        && IsPointerSink());
}

void VirtualControllerSink::Update(
  XrTime predictedDisplayTime,
  const std::optional<XrPosef>& leftAimPose,
  const std::optional<XrPosef>& rightAimPose,
  const ActionState& actionState) {
  mPredictedDisplayTime = predictedDisplayTime;

  mLeftHand.present = leftAimPose.has_value();
  if (mLeftHand.present) {
    mLeftHand.aimPose = OffsetPointerPose(*leftAimPose);
  }
  mRightHand.present = rightAimPose.has_value();
  if (mRightHand.present) {
    mRightHand.aimPose = OffsetPointerPose(*rightAimPose);
  }

  for (auto* hand: {&mLeftHand, &mRightHand}) {
    if (!hand->present) {
      continue;
    }
    SetControllerActions(hand, actionState);
  }
}

void VirtualControllerSink::SetControllerActions(
  ControllerState* controller,
  const ActionState& actions) {
  if (!IsActionSink()) {
    return;
  }
  if (UseDCSActions()) {
    SetDCSControllerActions(controller, actions);
    return;
  }
  if (UseMSFSActions()) {
    SetMSFSControllerActions(controller, actions);
    return;
  }

  static bool sFirstFail = true;
  if (sFirstFail) {
    sFirstFail = false;
    DebugPrint("Setting controller actions, but no binding set");
  }
}

void VirtualControllerSink::SetDCSControllerActions(
  ControllerState* hand,
  const ActionState& actionState) {
  hand->thumbstickX.changedSinceLastSync = true;
  hand->thumbstickY.changedSinceLastSync = true;

  if (actionState.mLeftClick) {
    hand->thumbstickY.currentState = -1.0f;
  } else if (actionState.mRightClick) {
    hand->thumbstickY.currentState = 1.0f;
  } else {
    hand->thumbstickY.currentState = 0.0f;
  }

  if (actionState.mDecreaseValue) {
    hand->thumbstickX.currentState = -1.0f;
  } else if (actionState.mIncreaseValue) {
    hand->thumbstickX.currentState = 1.0f;
  } else {
    hand->thumbstickX.currentState = 0.0f;
  }
}

void VirtualControllerSink::SetMSFSControllerActions(
  ControllerState* hand,
  const ActionState& actionState) {
  hand->triggerValue.changedSinceLastSync = true;
  hand->triggerValue.currentState = actionState.mLeftClick;
}

XrResult VirtualControllerSink::xrSyncActions(
  XrSession session,
  const XrActionsSyncInfo* syncInfo) {
  static bool sFirstRun = true;
  for (auto hand: {&mLeftHand, &mRightHand}) {
    const bool presenceChanged
      = sFirstRun || (hand->present != hand->presentLastSync);
    sFirstRun = false;
    hand->presentLastSync = hand->present;

    hand->squeezeValue.currentState = (hand->present ? 1.0f : 0.0f);
    hand->squeezeValue.isActive = hand->present;
    hand->squeezeValue.changedSinceLastSync = presenceChanged;

    hand->thumbstickTouch.currentState = (hand->present ? XR_TRUE : XR_FALSE);
    hand->thumbstickTouch.changedSinceLastSync = presenceChanged;
    hand->thumbstickTouch.isActive = hand->present;
    hand->triggerTouch = hand->thumbstickTouch;

    hand->thumbstickX.isActive = hand->present;
    hand->thumbstickY.isActive = hand->present;
    hand->triggerValue.isActive = hand->present;
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

std::string_view VirtualControllerSink::ResolvePath(XrPath path) {
  if (path == XR_NULL_PATH) {
    return {};
  }

  auto it = mPaths.find(path);
  if (it != mPaths.end()) {
    return it->second;
  }

  char buf[XR_MAX_PATH_LENGTH];
  uint32_t bufLen;
  if (
    mOpenXR->xrPathToString(mInstance, path, sizeof(buf), &bufLen, buf)
    != XR_SUCCESS) {
    return {};
  }

  std::string_view str {buf, bufLen - 1};
  mPaths[path] = str;

  if (str == gLeftHandPath) {
    mLeftHand.path = path;
  } else if (str == gRightHandPath) {
    mRightHand.path = path;
  }

  return mPaths.at(path);
}

XrResult VirtualControllerSink::xrGetCurrentInteractionProfile(
  XrSession session,
  XrPath path,
  XrInteractionProfileState* interactionProfile) {
  if (!mHaveSuggestedBindings) {
    return mOpenXR->xrGetCurrentInteractionProfile(
      session, path, interactionProfile);
  }
  // We need the side effect of populating m(Left|Right)Hand.path
  const auto pathStr = ResolvePath(path);

  if (Config::VerboseDebug >= 1) {
    DebugPrint("Requested interaction profile for {}", pathStr);
  }

  if (path == mLeftHand.path) {
    interactionProfile->interactionProfile
      = mLeftHand.present ? mProfilePath : XR_NULL_PATH;
    return XR_SUCCESS;
  }
  if (path == mRightHand.path) {
    interactionProfile->interactionProfile
      = mRightHand.present ? mProfilePath : XR_NULL_PATH;
    return XR_SUCCESS;
  }

  return mOpenXR->xrGetCurrentInteractionProfile(
    session, path, interactionProfile);
}

XrResult VirtualControllerSink::xrSuggestInteractionProfileBindings(
  XrInstance instance,
  const XrInteractionProfileSuggestedBinding* suggestedBindings) {
  {
    const auto interactionProfile
      = ResolvePath(suggestedBindings->interactionProfile);

    if (interactionProfile != Config::VirtualControllerInteractionProfilePath) {
      DebugPrint(
        "Profile '{}' does not match desired profile '{}'",
        interactionProfile,
        Config::VirtualControllerInteractionProfilePath);
      return mOpenXR->xrSuggestInteractionProfileBindings(
        instance, suggestedBindings);
    }
    DebugPrint(
      "Found desired profile '{}'",
      Config::VirtualControllerInteractionProfilePath);
    mProfilePath = suggestedBindings->interactionProfile;
  }

  for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; ++i) {
    const auto& it = suggestedBindings->suggestedBindings[i];
    const auto binding = ResolvePath(it.binding);
    mActionPaths[it.action] = binding;

    if (Config::VerboseDebug >= 2) {
      DebugPrint("Binding requested: {}", binding);
    }

    ControllerState* state;
    if (binding.starts_with(gLeftHandPath)) {
      state = &mLeftHand;
    } else if (binding.starts_with(gRightHandPath)) {
      state = &mRightHand;
    } else {
      continue;
    }

    if (IsPointerSink()) {
      if (binding.ends_with(gAimPosePath)) {
        state->aimActions.insert(it.action);
        continue;
      }

      if (binding.ends_with(gGripPosePath)) {
        state->gripActions.insert(it.action);
        continue;
      }

      // Partially cosmetic, also helps with 'is using this controller' in some
      // games
      if (binding.ends_with(gSqueezeValuePath)) {
        state->squeezeValueActions.insert(it.action);
        continue;
      }

      // Cosmetic
      if (binding.ends_with(gThumbstickTouchPath)) {
        state->thumbstickTouchActions.insert(it.action);
        continue;
      }

      if (binding.ends_with(gTriggerTouchPath)) {
        state->triggerTouchActions.insert(it.action);
        continue;
      }
    }

    if (IsActionSink()) {
      if (binding.ends_with(gThumbstickXPath)) {
        state->thumbstickXActions.insert(it.action);
        continue;
      }

      if (binding.ends_with(gThumbstickYPath)) {
        state->thumbstickYActions.insert(it.action);
        continue;
      }

      if (binding.ends_with(gTriggerTouchPath)) {
        state->triggerTouchActions.insert(it.action);
        continue;
      }

      if (binding.ends_with(gTriggerValuePath)) {
        state->triggerValueActions.insert(it.action);
        continue;
      }
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

  const auto path = createInfo->subactionPath;
  ResolvePath(path);
  for (auto hand: {&mLeftHand, &mRightHand}) {
    if (path != XR_NULL_PATH && path != hand->path) {
      continue;
    }
    if (hand->aimActions.contains(createInfo->action)) {
      hand->aimSpace = *space;
      DebugPrint(
        "Found aim space: {:#016x}", reinterpret_cast<uintptr_t>(*space));
      return XR_SUCCESS;
    }

    if (hand->gripActions.contains(createInfo->action)) {
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

    if (hand->triggerTouchActions.contains(action)) {
      *state = hand->triggerTouch;
      return XR_SUCCESS;
    }

    if (hand->triggerValueActions.contains(action)) {
      *state = hand->triggerValue;
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

    if (hand->thumbstickXActions.contains(action)) {
      *state = hand->thumbstickX;
      return XR_SUCCESS;
    }

    if (hand->thumbstickYActions.contains(action)) {
      *state = hand->thumbstickY;
      return XR_SUCCESS;
    }

    if (hand->triggerValueActions.contains(action)) {
      *state = {
        .type = XR_TYPE_ACTION_STATE_FLOAT,
        .currentState = hand->triggerValue.currentState ? 1.0f : 0.0f,
        .changedSinceLastSync = hand->triggerValue.changedSinceLastSync,
        .lastChangeTime = hand->triggerValue.lastChangeTime,
        .isActive = hand->triggerValue.isActive,
      };
      return XR_SUCCESS;
    }
  }

  return mOpenXR->xrGetActionStateFloat(session, getInfo, state);
}

XrResult VirtualControllerSink::xrGetActionStatePose(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStatePose* state) {
  ResolvePath(getInfo->subactionPath);
  const auto action = getInfo->action;

  for (auto hand: {&mLeftHand, &mRightHand}) {
    if (
      hand->aimActions.contains(action) || hand->gripActions.contains(action)) {
      if (
        getInfo->subactionPath != XR_NULL_PATH
        && getInfo->subactionPath != hand->path) {
        continue;
      }

      state->isActive = hand->present ? XR_TRUE : XR_FALSE;
      return XR_SUCCESS;
    }
  }
  return mOpenXR->xrGetActionStatePose(session, getInfo, state);
}

XrPosef VirtualControllerSink::OffsetPointerPose(const XrPosef& original) {
  if (!Config::IsRaycastOrientation()) {
    return original;
  }
  const auto nearDistance
    = Vector3(original.position.x, original.position.y, original.position.z)
        .Length();
  const auto nearFarDistance = Config::VRFarDistance - nearDistance;
  const auto rx = std::atan2f(Config::VRVerticalOffset, nearFarDistance);

  const XrVector3f position {
    original.position.x,
    original.position.y + Config::VRVerticalOffset,
    original.position.z,
  };

  const auto q = Quaternion(
                   original.orientation.x,
                   original.orientation.y,
                   original.orientation.z,
                   original.orientation.w)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitX, -rx);
  const XrQuaternionf orientation {q.x, q.y, q.z, q.w};

  return {orientation, position};
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
      *location = {XR_TYPE_SPACE_LOCATION};
      return XR_SUCCESS;
    }

    mOpenXR->xrLocateSpace(mViewSpace, baseSpace, time, location);

    const auto viewPose = location->pose;

    if (space == hand.aimSpace) {
      location->pose = hand.aimPose * viewPose;
      return XR_SUCCESS;
    }

    // Just experimentation; use PointCtrl to calibrate this: as it's
    // a 2D source, the 'laser' should always be straight line
    auto aimToGripQ = Quaternion::CreateFromAxisAngle(
                        Vector3::UnitX, std::numbers::pi_v<float> * 0.23f)
      * Quaternion::CreateFromAxisAngle(
                        Vector3::UnitY,
                        (hand.hand == XR_HAND_LEFT_EXT ? 1 : -1)
                          * std::numbers::pi_v<float> * 0.1f);

    XrPosef aimToGrip {
      .orientation = {aimToGripQ.x, aimToGripQ.y, aimToGripQ.z, aimToGripQ.w},
    };

    const auto handPose = aimToGrip * hand.aimPose;

    location->pose = handPose * viewPose;

    return XR_SUCCESS;
  }

  return mOpenXR->xrLocateSpace(space, baseSpace, time, location);
}

}// namespace HandTrackedCockpitClicking
