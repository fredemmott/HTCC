// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT

#include "VirtualControllerSink.h"

#include <directxtk/SimpleMath.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <chrono>
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
  return Config::PointerSink == PointerSink::VirtualVRController;
}

static bool IsActionSink(ActionSink actionSink) {
  return (actionSink == ActionSink::VirtualVRController)
    || ((actionSink == ActionSink::MatchPointerSink)
        && VirtualControllerSink::IsPointerSink());
}

static bool IsClickActionSink() {
  return IsActionSink(Config::ClickActionSink);
}

static bool IsScrollActionSink() {
  return IsActionSink(Config::ScrollActionSink);
}

bool VirtualControllerSink::IsActionSink() {
  return IsClickActionSink() || IsScrollActionSink();
}

void VirtualControllerSink::Update(
  const FrameInfo& info,
  const InputState& leftHand,
  const InputState& rightHand) {
  UpdateHand(info, leftHand, &mLeftController);
  UpdateHand(info, rightHand, &mRightController);
}

static bool WorldLockOrientation() {
  switch (Config::VRControllerPointerSinkWorldLock) {
    case VRControllerPointerSinkWorldLock::Nothing:
      return false;
    case VRControllerPointerSinkWorldLock::Orientation:
    case VRControllerPointerSinkWorldLock::OrientationAndSoftPosition:
      return true;
  }
  __assume(false);
}

std::optional<XrPosef> VirtualControllerSink::GetInputPose(
  const FrameInfo& frameInfo,
  const InputState& hand,
  ControllerState* controller) {
  if (hand.mActions.Any() && !hand.mPose) {
    return controller->savedAimPose;
  }

  if (!hand.mPose) {
    controller->savedAimPose = {};
    return {};
  }

  const auto lastDirection = controller->mPreviousFrameDirection;
  controller->mPreviousFrameDirection = hand.mDirection;

  if (hand.mDirection && lastDirection) {
    // Assuming that identical input values are an 'out of range' signal,
    // so maintain the world lock even if moved.
    //
    // Fuzzing to 1.0e-7 because floats.
    if (lastDirection->x != 0.0f) {
      const auto diff = std::abs(hand.mDirection->x / lastDirection->x);
      if (diff < 1.0e-7) {
        return controller->savedAimPose;
      }
    }
    if (lastDirection->y != 0.0f) {
      const auto diff = std::abs(hand.mDirection->y / lastDirection->y);
      if (diff < 1.0e-7) {
        return controller->savedAimPose;
      }
    }
  }

  auto inputPose = OffsetPointerPose(frameInfo, *hand.mPose);
  if (!(hand.mActions.Any() && controller->savedAimPose)) {
    controller->savedAimPose = inputPose;
    controller->mUnlockedPosition = false;
    return inputPose;
  }

  if (WorldLockOrientation()) {
    inputPose.orientation = controller->savedAimPose->orientation;
  }

  if (
    Config::VRControllerPointerSinkWorldLock
    != VRControllerPointerSinkWorldLock::OrientationAndSoftPosition) {
    return inputPose;
  }

  if (controller->mUnlockedPosition) {
    return inputPose;
  }

  const auto a = XrVecToSM(inputPose.position);
  const auto b = XrVecToSM(controller->savedAimPose->position);
  const auto distance = std::abs(Vector3::Distance(a, b));
  if (distance < Config::VRControllerPointerSinkSoftWorldLockDistance) {
    inputPose.position = controller->savedAimPose->position;
  } else {
    controller->mUnlockedPosition = true;
  }

  return inputPose;
}

void VirtualControllerSink::UpdateHand(
  const FrameInfo& frameInfo,
  const InputState& hand,
  ControllerState* controller) {
  auto aimPose = GetInputPose(frameInfo, hand, controller);
  if (!aimPose) {
    controller->present = false;
    return;
  }

  controller->aimPose = *aimPose;
  controller->present = true;

  SetControllerActions(
    frameInfo.mPredictedDisplayTime, hand.mActions, controller);
}

void VirtualControllerSink::SetControllerActions(
  XrTime predictedDisplayTime,
  const ActionState& hand,
  ControllerState* controller) {
  if (!IsActionSink()) {
    return;
  }
  if (UseDCSActions()) {
    SetDCSControllerActions(predictedDisplayTime, hand, controller);
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
  XrTime predictedDisplayTime,
  const ActionState& hand,
  ControllerState* controller) {
  if (IsClickActionSink()) {
    controller->thumbstickY.changedSinceLastSync = true;
    if (hand.mPrimary) {
      controller->thumbstickY.currentState = -1.0f;
    } else if (hand.mSecondary) {
      controller->thumbstickY.currentState = 1.0f;
    } else {
      controller->thumbstickY.currentState = 0.0f;
    }
  }

  if (!IsScrollActionSink()) {
    return;
  }

  using ValueChange = ActionState::ValueChange;
  controller->thumbstickX.changedSinceLastSync = true;
  if (hand.mValueChange != controller->mValueChange) {
    controller->mValueChange = hand.mValueChange;
    controller->mValueChangeStartAt = predictedDisplayTime;
  }

  if (hand.mValueChange == ValueChange::None) {
    controller->thumbstickX.currentState = 0.0f;
    return;
  }

  // DCS ignores values below 0.3, so let's just make it 3-stage
  const auto minimumRate = (1.0f / 3.0f);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::nanoseconds(
                      predictedDisplayTime - controller->mValueChangeStartAt))
                    .count();
  const auto rate = std::clamp<float>(
    minimumRate
      * (1 + (ms / Config::VRControllerScrollAccelerationDelayMilliseconds)),
    0.0f,
    1.0f);

  switch (hand.mValueChange) {
    case ValueChange::Decrease:
      controller->thumbstickX.currentState = -rate;
      break;
    case ValueChange::Increase:
      controller->thumbstickX.currentState = rate;
      break;
    case ValueChange::None:
      // unreachable
      break;
  }
}

void VirtualControllerSink::SetMSFSControllerActions(
  XrTime predictedDisplayTime,
  const ActionState& hand,
  ControllerState* controller) {
  using ValueChange = ActionState::ValueChange;
  const auto rawPrimary = IsClickActionSink() && hand.mPrimary;
  const auto rawSecondary = IsClickActionSink() && hand.mSecondary;
  const auto rawValueChange
    = IsScrollActionSink() ? hand.mValueChange : ValueChange::None;

  const auto emulatePrimaryInteraction
    = (!rawPrimary) && (rawSecondary || rawValueChange != ValueChange::None);
  const auto hadPrimaryInteraction = controller->triggerValue.currentState;

  controller->triggerValue.changedSinceLastSync = true;
  controller->triggerValue.currentState
    = rawPrimary || emulatePrimaryInteraction;
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

  if (rawSecondary && !skipThisFrame) {
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
    switch (rawValueChange) {
      case ValueChange::None:
        controller->mRotationDirection = Rotation::None;
        if (!rawPrimary) {
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
    controller->mLastRotationAt = predictedDisplayTime;
  }

  if (controller->mRotationDirection != Rotation::None) {
    const auto seconds
      = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::nanoseconds(
            predictedDisplayTime - controller->mLastRotationAt))
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
    controller->mLastRotationAt = predictedDisplayTime;
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

    switch (Config::VRControllerGripSqueeze) {
      case VRControllerGripSqueeze::Never:
        hand->squeezeValue.currentState = 0.0f;
        break;
      case VRControllerGripSqueeze::WhenTracking:
        hand->squeezeValue.currentState = (hand->present ? 1.0f : 0.0f);
        break;
    }
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
  const auto interactionProfile
    = ResolvePath(suggestedBindings->interactionProfile);
  {
    if (interactionProfile != Config::VirtualControllerInteractionProfilePath) {
      DebugPrint(
        "Profile '{}' does not match desired profile '{}'",
        interactionProfile,
        Config::VirtualControllerInteractionProfilePath);
      return XR_SUCCESS;
    }
    DebugPrint(
      "Found desired profile '{}'",
      Config::VirtualControllerInteractionProfilePath);
    mProfilePath = suggestedBindings->interactionProfile;
  }

  for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; ++i) {
    const auto& it = suggestedBindings->suggestedBindings[i];
    this->AddBinding(it.binding, it.action);
  }
  mHaveSuggestedBindings = true;

  return XR_SUCCESS;
}

XrResult VirtualControllerSink::xrCreateAction(
  XrActionSet actionSet,
  const XrActionCreateInfo* createInfo,
  XrAction* action) {
  const auto ret = mOpenXR->xrCreateAction(actionSet, createInfo, action);
  if (!XR_SUCCEEDED(ret)) {
    return ret;
  }

  for (uint32_t i = 0; i < createInfo->countSubactionPaths; ++i) {
    AddBinding(createInfo->subactionPaths[i], *action);
  }

  return ret;
}

void VirtualControllerSink::AddBinding(XrPath path, XrAction action) {
  const auto binding = ResolvePath(path);

  ControllerState* state {nullptr};
  if (binding.starts_with(gLeftHandPath)) {
    state = &mLeftController;
  } else if (binding.starts_with(gRightHandPath)) {
    state = &mRightController;
  } else {
    return;
  }

  if (IsPointerSink()) {
    if (binding.ends_with(gAimPosePath)) {
      state->aimActions.emplace(action);
      if (mActionSpaces.contains(action)) {
        const auto actionSpaces = mActionSpaces.at(action);
        state->aimSpaces.insert(actionSpaces.begin(), actionSpaces.end());
      }
      DebugPrint("Aim action found");
      return;
    }

    if (binding.ends_with(gGripPosePath)) {
      state->gripActions.emplace(action);
      if (mActionSpaces.contains(action)) {
        const auto actionSpaces = mActionSpaces.at(action);
        state->gripSpaces.insert(actionSpaces.begin(), actionSpaces.end());
      }
      DebugPrint("Grip action found");
      return;
    }

    // Partially cosmetic, also helps with 'is using this controller' in
    // some games
    if (binding.ends_with(gSqueezeValuePath)) {
      state->squeezeValueActions.emplace(action);
      DebugPrint("Squeeze action found");
      return;
    }

    // Cosmetic
    if (binding.ends_with(gThumbstickTouchPath)) {
      state->thumbstickTouchActions.emplace(action);
      DebugPrint("Thumbstick touch action found");
      return;
    }

    if (binding.ends_with(gTriggerTouchPath)) {
      state->triggerTouchActions.emplace(action);
      DebugPrint("Trigger touch action found");
      return;
    }
  }

  if (IsActionSink()) {
    if (binding.ends_with(gThumbstickXPath)) {
      state->thumbstickXActions.emplace(action);
      DebugPrint("Thumbstick X action found");
      return;
    }

    if (binding.ends_with(gThumbstickYPath)) {
      state->thumbstickYActions.emplace(action);
      DebugPrint("Thumbstick Y action found");
      return;
    }

    if (binding.ends_with(gTriggerTouchPath)) {
      state->triggerTouchActions.emplace(action);
      DebugPrint("Trigger touch action found");
      return;
    }

    if (binding.ends_with(gTriggerValuePath)) {
      state->triggerValueActions.emplace(action);
      DebugPrint("Trigger value action found");
      return;
    }
  }
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

  // It's fine to call xrCreateActionSpace before the bindings have
  // been suggested - so, we need to track the space, even if we have no
  // reason yet to think that the action is relevant
  mActionSpaces[createInfo->action].emplace(*space);

  const auto path = createInfo->subactionPath;
  if (path) {
    // Populate hand.path
    ResolvePath(path);
  }

  for (auto hand: {&mLeftController, &mRightController}) {
    if (hand->aimActions.contains(createInfo->action)) {
      if (path && path != hand->path) {
        DebugPrint(
          "Created space for aim action, but with different subactionPath");
        continue;
      }
      hand->aimSpaces.emplace(*space);
      DebugPrint(
        "Found aim space: {:#016x}", reinterpret_cast<uintptr_t>(*space));
      return XR_SUCCESS;
    }

    if (hand->gripActions.contains(createInfo->action)) {
      if (path && path != hand->path) {
        DebugPrint(
          "Created space for grip action, but with different subactionPath");
        continue;
      }
      DebugPrint(
        "Found grip space: {:#016x}", reinterpret_cast<uintptr_t>(*space));
      hand->gripSpaces.emplace(*space);
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
    const bool isAimSpace = hand.aimSpaces.contains(space);
    const bool isGripSpace = hand.gripSpaces.contains(space);
    if (!(isAimSpace || isGripSpace)) {
      TraceLoggingWrite(gTraceProvider, "xrLocateSpace_notAimOrGripSpace");
      continue;
    }

    if (!hand.present) {
      *location = {XR_TYPE_SPACE_LOCATION};
      TraceLoggingWrite(gTraceProvider, "xrLocateSpace_handNotPresent");
      return XR_SUCCESS;
    }

    const auto nextRet
      = mOpenXR->xrLocateSpace(mLocalSpace, baseSpace, time, location);
    if (!XR_SUCCEEDED(nextRet)) {
      TraceLoggingWrite(
        gTraceProvider,
        "xrLocateSpace_failedNext",
        TraceLoggingValue(static_cast<const int64_t>(nextRet), "XrResult"));
      return nextRet;
    }

    constexpr auto poseValid = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT
      | XR_SPACE_LOCATION_POSITION_VALID_BIT;
    constexpr auto poseTracked = XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT
      | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;

    const auto spacePose = ((location->locationFlags & poseValid) == poseValid)
      ? location->pose
      : XR_POSEF_IDENTITY;
    const auto aimPose = hand.aimPose;

    if (isAimSpace) {
      location->pose = aimPose * spacePose;
      location->locationFlags |= poseValid | poseTracked;
      TraceLoggingWrite(gTraceProvider, "xrLocateSpace_handAimSpace");
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
    location->locationFlags |= poseValid | poseTracked;
    TraceLoggingWrite(gTraceProvider, "xrLocateSpace_handGripSpace");

    return XR_SUCCESS;
  }

  return mOpenXR->xrLocateSpace(space, baseSpace, time, location);
}

}// namespace HandTrackedCockpitClicking
