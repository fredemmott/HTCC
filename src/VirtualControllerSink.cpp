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

#include <limits>
#include <numbers>

#include "Environment.h"
#include "InputState.h"
#include "openxr.h"

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
  XrReferenceSpaceCreateInfo referenceSpace {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .next = nullptr,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
    .poseInReferenceSpace = XR_POSEF_IDENTITY,
  };

  const auto nextResult
    = openXR->xrCreateReferenceSpace(session, &referenceSpace, &mLocalSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create local space: {}", nextResult);
    return;
  }

  DebugPrint(
    "Initialized virtual VR controller - PointerSink: {}; ActionSink: {}",
    IsPointerSink(),
    IsActionSink());
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
  const FrameInfo& info,
  const InputState& leftHand,
  const InputState& rightHand) {
  UpdateHand(info, leftHand, &mLeftController);
  UpdateHand(info, rightHand, &mRightController);
}

void VirtualControllerSink::UpdateHand(
  const FrameInfo& frameInfo,
  const InputState& hand,
  ControllerState* controller) {
  if (!hand.mPose) {
    controller->present = false;
    return;
  }
  controller->present = true;

  auto inputPose = OffsetPointerPose(frameInfo, *hand.mPose);
  const auto haveAction = hand.AnyInteraction();
  if (
    haveAction
    && Config::VRControllerActionSinkWorldLock
      == VRControllerActionSinkWorldLock::Orientation) {
    inputPose.orientation = controller->savedAimPose.orientation;
  } else if (!haveAction) {
    controller->haveAction = false;
    controller->savedAimPose = inputPose;
  } else if (!controller->haveAction) {
    controller->haveAction = true;
  }
  controller->aimPose = {inputPose};

  SetControllerActions(frameInfo.mPredictedDisplayTime, hand, controller);
}

void VirtualControllerSink::SetControllerActions(
  XrTime predictedDisplayTime,
  const InputState& hand,
  ControllerState* controller) {
  if (!IsActionSink()) {
    return;
  }
  if (UseDCSActions()) {
    SetDCSControllerActions(hand, controller);
    return;
  }
  if (UseMSFSActions()) {
    SetMSFSControllerActions(predictedDisplayTime, hand, controller);
    return;
  }

  static bool sFirstFail = true;
  if (sFirstFail) {
    sFirstFail = false;
    DebugPrint("Setting controller actions, but no binding set");
  }
}

void VirtualControllerSink::SetDCSControllerActions(
  const InputState& hand,
  ControllerState* controller) {
  controller->thumbstickX.changedSinceLastSync = true;
  controller->thumbstickY.changedSinceLastSync = true;

  if (hand.mPrimaryInteraction) {
    controller->thumbstickY.currentState = -1.0f;
  } else if (hand.mSecondaryInteraction) {
    controller->thumbstickY.currentState = 1.0f;
  } else {
    controller->thumbstickY.currentState = 0.0f;
  }

  using ValueChange = InputState::ValueChange;
  switch (hand.mValueChange) {
    case ValueChange::Decrease:
      controller->thumbstickX.currentState = -1.0f;
      break;
    case ValueChange::Increase:
      controller->thumbstickX.currentState = 1.0f;
      break;
    case ValueChange::None:
      controller->thumbstickX.currentState = 0.0f;
      break;
  }
}

void VirtualControllerSink::SetMSFSControllerActions(
  XrTime predictedDisplayTime,
  const InputState& hand,
  ControllerState* controller) {
  using ValueChange = InputState::ValueChange;
  const auto emulatePrimaryInteraction = (!hand.mPrimaryInteraction)
    && (hand.mSecondaryInteraction || hand.mValueChange != ValueChange::None);
  const auto hadPrimaryInteraction = controller->triggerValue.currentState;

  controller->triggerValue.changedSinceLastSync = true;
  controller->triggerValue.currentState
    = hand.mPrimaryInteraction || emulatePrimaryInteraction;
  controller->triggerValue.lastChangeTime = predictedDisplayTime;

  // Press trigger for one frame so MSFS recognizes it before the
  // other action
  if (emulatePrimaryInteraction && !hadPrimaryInteraction) {
    constexpr auto delayMS = std::chrono::milliseconds(100);
    constexpr auto delayUS
      = std::chrono::duration_cast<std::chrono::nanoseconds>(delayMS).count();
    controller->mBlockSecondaryActionsUntil = predictedDisplayTime + delayUS;
  }
  const auto skipThisFrame
    = predictedDisplayTime < controller->mBlockSecondaryActionsUntil;

  if (hand.mSecondaryInteraction && !skipThisFrame) {
    // 'push' forward
    const auto worldOffset = Vector3::Transform(
      {0.0f, 0.0f, -0.02f}, XrQuatToSM(controller->aimPose.orientation));
    auto& o = controller->aimPose.position;
    o.x += worldOffset.x;
    o.y += worldOffset.y;
    o.z += worldOffset.z;
  }

  // Just increase/decrease value from here
  const auto oldRotationDirection = controller->mRotationDirection;
  if (skipThisFrame) {
    controller->mRotationDirection = Rotation::None;
  } else {
    switch (hand.mValueChange) {
      case ValueChange::None:
        controller->mRotationDirection = Rotation::None;
        if (!hand.mPrimaryInteraction) {
          controller->mRotationAngle = 0.0f;
        }
        break;
      case ValueChange::Increase:
        controller->mRotationDirection = Rotation::Clockwise;
        break;
      case ValueChange::Decrease:
        controller->mRotationDirection = Rotation::CounterClockwise;
        break;
    }
  }

  if (controller->mRotationDirection != oldRotationDirection) {
    controller->mLastRotatedAt = predictedDisplayTime;
  }

  if (controller->mRotationDirection != Rotation::None) {
    const auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::nanoseconds(
                             predictedDisplayTime - controller->mLastRotatedAt))
                           .count()
      / 1000.0f;

    const auto secondsPerRotation
      = Config::VRControllerActionSinkSecondsPerRotation;
    const auto rotations = seconds / secondsPerRotation;
    const auto radians = rotations * 2 * std::numbers::pi_v<float>;
    if (controller->mRotationDirection == Rotation::Clockwise) {
      controller->mRotationAngle -= radians;
    } else {
      controller->mRotationAngle += radians;
    }
    controller->mLastRotatedAt = predictedDisplayTime;
  }

  if (
    std::abs(controller->mRotationAngle)
    < std::numeric_limits<float>::epsilon()) {
    return;
  }

  const auto quat = Quaternion::CreateFromAxisAngle(
    Vector3::UnitZ, controller->mRotationAngle);
  controller->aimPose.orientation
    = SMQuatToXr(quat * XrQuatToSM(controller->aimPose.orientation));
}

XrResult VirtualControllerSink::xrSyncActions(
  XrSession session,
  const XrActionsSyncInfo* syncInfo) {
  static bool sFirstRun = true;
  for (auto hand: {&mLeftController, &mRightController}) {
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
    mLeftController.present != mLeftController.presentLastPollEvent
    || mRightController.present != mRightController.presentLastPollEvent)) {
    mLeftController.presentLastPollEvent = mLeftController.present;
    mRightController.presentLastPollEvent = mRightController.present;
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
  if (!mOpenXR->check_xrPathToString(
        mInstance, path, sizeof(buf), &bufLen, buf)) {
    return {};
  }

  std::string_view str {buf, bufLen - 1};
  mPaths[path] = str;

  if (str == gLeftHandPath) {
    mLeftController.path = path;
  } else if (str == gRightHandPath) {
    mRightController.path = path;
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

  if (path == mLeftController.path) {
    interactionProfile->interactionProfile
      = mLeftController.present ? mProfilePath : XR_NULL_PATH;
    return XR_SUCCESS;
  }
  if (path == mRightController.path) {
    interactionProfile->interactionProfile
      = mRightController.present ? mProfilePath : XR_NULL_PATH;
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
      state = &mLeftController;
    } else if (binding.starts_with(gRightHandPath)) {
      state = &mRightController;
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

      // Partially cosmetic, also helps with 'is using this controller' in
      // some games
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
  for (auto hand: {&mLeftController, &mRightController}) {
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
  ResolvePath(getInfo->subactionPath);
  const auto action = getInfo->action;

  for (auto hand: {&mLeftController, &mRightController}) {
    if (
      getInfo->subactionPath != XR_NULL_PATH
      && getInfo->subactionPath != hand->path) {
      continue;
    }

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

  for (auto hand: {&mLeftController, &mRightController}) {
    if (
      getInfo->subactionPath != XR_NULL_PATH
      && getInfo->subactionPath != hand->path) {
      continue;
    }

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

  for (auto hand: {&mLeftController, &mRightController}) {
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

XrPosef VirtualControllerSink::OffsetPointerPose(
  const FrameInfo& frameInfo,
  const XrPosef& handInLocal) {
  const auto handInView = handInLocal * frameInfo.mLocalInView;

  const auto nearDistance
    = Vector3(
        handInView.position.x, handInView.position.y, handInView.position.z)
        .Length();
  const auto nearFarDistance = Config::VRFarDistance - nearDistance;

  const auto rx = std::atan2f(Config::VRVerticalOffset, nearFarDistance);

  const XrVector3f position {
    handInView.position.x,
    handInView.position.y + Config::VRVerticalOffset,
    handInView.position.z,
  };

  const auto q = Quaternion(
                   handInView.orientation.x,
                   handInView.orientation.y,
                   handInView.orientation.z,
                   handInView.orientation.w)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitX, -rx);
  const XrQuaternionf orientation {q.x, q.y, q.z, q.w};

  return XrPosef {orientation, position} * frameInfo.mViewInLocal;
}

XrResult VirtualControllerSink::xrLocateSpace(
  XrSpace space,
  XrSpace baseSpace,
  XrTime time,
  XrSpaceLocation* location) {
  for (const ControllerState& hand: {mLeftController, mRightController}) {
    if (space != hand.aimSpace && space != hand.gripSpace) {
      continue;
    }

    if (!hand.present) {
      *location = {XR_TYPE_SPACE_LOCATION};
      return XR_SUCCESS;
    }

    mOpenXR->xrLocateSpace(mLocalSpace, baseSpace, time, location);

    const auto spacePose = location->pose;
    const auto aimPose = hand.aimPose;

    if (space == hand.aimSpace) {
      location->pose = aimPose * spacePose;
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

    const auto handPose = aimToGrip * aimPose;

    location->pose = handPose * spacePose;

    return XR_SUCCESS;
  }

  return mOpenXR->xrLocateSpace(space, baseSpace, time, location);
}

}// namespace HandTrackedCockpitClicking
