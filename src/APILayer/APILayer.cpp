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

#include <directxtk/SimpleMath.h>
#include <loader_interfaces.h>

#include <memory>
#include <string>
#include <vector>

#include "DebugPrint.h"
#include "Environment.h"
#include "HandTrackingSource.h"
#include "OpenXRNext.h"
#include "PointCtrlSource.h"
#include "VirtualControllerSink.h"
#include "VirtualTouchScreenSink.h"
#include "openxr.h"

using namespace DirectX::SimpleMath;

namespace HandTrackedCockpitClicking {

APILayer::APILayer(
  XrInstance instance,
  XrSession session,
  const std::shared_ptr<OpenXRNext>& next)
  : mOpenXR(next), mInstance(instance) {
  DebugPrint("{}()", __FUNCTION__);

  if (!Environment::Have_XR_KHR_win32_convert_performance_counter_time) {
    return;
  }


  auto oxr = next.get();

  XrReferenceSpaceCreateInfo referenceSpace {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .next = nullptr,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
    .poseInReferenceSpace = XR_POSEF_IDENTITY,
  };

  if (!oxr->check_xrCreateReferenceSpace(
        session, &referenceSpace, &mViewSpace)) {
    DebugPrint("Failed to create view space");
    return;
  }
  referenceSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  if (!oxr->check_xrCreateReferenceSpace(
        session, &referenceSpace, &mLocalSpace)) {
    DebugPrint("Failed to create world space");
    return;
  }

  if (Environment::Have_XR_EXT_HandTracking) {
    mHandTracking = std::make_unique<HandTrackingSource>(
      next, instance, session, mViewSpace, mLocalSpace);
  }
  mPointCtrl = std::make_unique<PointCtrlSource>();

  if (
    VirtualControllerSink::IsActionSink()
    || VirtualControllerSink::IsPointerSink()) {
    mVirtualController = std::make_unique<VirtualControllerSink>(
      next, instance, session, mViewSpace);
  }

  DebugPrint("Fully initialized.");
}

APILayer::~APILayer() {
  if (mViewSpace) {
    mOpenXR->xrDestroySpace(mViewSpace);
  }
  if (mLocalSpace) {
    mOpenXR->xrDestroySpace(mLocalSpace);
  }
}

XrResult APILayer::xrSuggestInteractionProfileBindings(
  XrInstance instance,
  const XrInteractionProfileSuggestedBinding* suggestedBindings) {
  if (mVirtualController) {
    return mVirtualController->xrSuggestInteractionProfileBindings(
      instance, suggestedBindings);
  }
  return mOpenXR->xrSuggestInteractionProfileBindings(
    instance, suggestedBindings);
}

XrResult APILayer::xrGetActionStateBoolean(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateBoolean* state) {
  if (mVirtualController) {
    return mVirtualController->xrGetActionStateBoolean(session, getInfo, state);
  }
  return mOpenXR->xrGetActionStateBoolean(session, getInfo, state);
}

XrResult APILayer::xrGetActionStateFloat(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateFloat* state) {
  if (mVirtualController) {
    return mVirtualController->xrGetActionStateFloat(session, getInfo, state);
  }
  return mOpenXR->xrGetActionStateFloat(session, getInfo, state);
}

XrResult APILayer::xrGetActionStatePose(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStatePose* state) {
  if (mVirtualController) {
    return mVirtualController->xrGetActionStatePose(session, getInfo, state);
  }
  return mOpenXR->xrGetActionStatePose(session, getInfo, state);
}

XrResult APILayer::xrLocateSpace(
  XrSpace space,
  XrSpace baseSpace,
  XrTime time,
  XrSpaceLocation* location) {
  if (mVirtualController) {
    return mVirtualController->xrLocateSpace(space, baseSpace, time, location);
  }
  return mOpenXR->xrLocateSpace(space, baseSpace, time, location);
}

XrResult APILayer::xrSyncActions(
  XrSession session,
  const XrActionsSyncInfo* syncInfo) {
  if (mVirtualController) {
    return mVirtualController->xrSyncActions(session, syncInfo);
  }
  return mOpenXR->xrSyncActions(session, syncInfo);
}

XrResult APILayer::xrPollEvent(
  XrInstance instance,
  XrEventDataBuffer* eventData) {
  if (mVirtualController) {
    return mVirtualController->xrPollEvent(instance, eventData);
  }
  return mOpenXR->xrPollEvent(instance, eventData);
}

XrResult APILayer::xrGetCurrentInteractionProfile(
  XrSession session,
  XrPath topLevelUserPath,
  XrInteractionProfileState* interactionProfile) {
  if (mVirtualController) {
    return mVirtualController->xrGetCurrentInteractionProfile(
      session, topLevelUserPath, interactionProfile);
  }
  return mOpenXR->xrGetCurrentInteractionProfile(
    session, topLevelUserPath, interactionProfile);
}

XrResult APILayer::xrCreateAction(
  XrActionSet actionSet,
  const XrActionCreateInfo* createInfo,
  XrAction* space) {
  if (mVirtualController) {
    return mVirtualController->xrCreateAction(actionSet, createInfo, space);
  }
  return mOpenXR->xrCreateAction(actionSet, createInfo, space);
}

XrResult APILayer::xrCreateActionSpace(
  XrSession session,
  const XrActionSpaceCreateInfo* createInfo,
  XrSpace* space) {
  if (mVirtualController) {
    return mVirtualController->xrCreateActionSpace(session, createInfo, space);
  }
  return mOpenXR->xrCreateActionSpace(session, createInfo, space);
}

// Report to higher layers and apps that OpenXR Hand Tracking is unavailable;
// HTCC should be the only thing using it.
XrResult APILayer::xrGetSystemProperties(
  XrInstance instance,
  XrSystemId systemId,
  XrSystemProperties* properties) {
  const auto nextResult
    = mOpenXR->xrGetSystemProperties(instance, systemId, properties);
  if (!XR_SUCCEEDED(nextResult)) {
    return nextResult;
  }

  auto next = reinterpret_cast<XrBaseOutStructure*>(properties->next);
  while (next) {
    if (next->type == XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT) {
      auto htp = reinterpret_cast<XrSystemHandTrackingPropertiesEXT*>(next);
      htp->supportsHandTracking = XR_FALSE;
    }
    next = reinterpret_cast<XrBaseOutStructure*>(next->next);
  }

  return nextResult;
}

XrResult APILayer::xrCreateHandTrackerEXT(
  XrSession session,
  const XrHandTrackerCreateInfoEXT* createInfo,
  XrHandTrackerEXT* handTracker) {
  return XR_ERROR_FEATURE_UNSUPPORTED;
}

XrResult APILayer::xrWaitFrame(
  XrSession session,
  const XrFrameWaitInfo* frameWaitInfo,
  XrFrameState* state) {
  const auto nextResult = mOpenXR->xrWaitFrame(session, frameWaitInfo, state);
  if (nextResult != XR_SUCCESS) {
    return nextResult;
  }

  FrameInfo frameInfo(
    mOpenXR.get(),
    mInstance,
    mLocalSpace,
    mViewSpace,
    state->predictedDisplayTime);

  if (
    (!mVirtualTouchScreen)
    && (VirtualTouchScreenSink::IsActionSink() || VirtualTouchScreenSink::IsPointerSink())) {
    mVirtualTouchScreen = std::make_unique<VirtualTouchScreenSink>(
      mOpenXR, session, state->predictedDisplayTime, mViewSpace);
  }

  InputState leftHand {XR_HAND_LEFT_EXT};
  InputState rightHand {XR_HAND_RIGHT_EXT};
  const auto pointerMode
    = (Config::PointerSink == PointerSink::VirtualTouchScreen)
    ? PointerMode::Direction
    : PointerMode::Pose;

  if (mHandTracking) {
    const auto [l, r] = mHandTracking->Update(
      (Config::PointerSource == PointerSource::OpenXRHandTracking)
        ? pointerMode
        : PointerMode::None,
      frameInfo);
    if (Config::PointerSource == PointerSource::OpenXRHandTracking) {
      leftHand.mPose = l.mPose;
      leftHand.mDirection = l.mDirection;
      rightHand.mPose = r.mPose;
      rightHand.mDirection = r.mDirection;
    }
    if (Config::PinchToClick) {
      leftHand.mActions.mPrimary = l.mActions.mPrimary;
      leftHand.mActions.mSecondary = l.mActions.mSecondary;
      rightHand.mActions.mPrimary = r.mActions.mPrimary;
      rightHand.mActions.mSecondary = r.mActions.mSecondary;
    }
    if (Config::PinchToScroll) {
      leftHand.mActions.mValueChange = l.mActions.mValueChange;
      rightHand.mActions.mValueChange = r.mActions.mValueChange;
    }
  }

  if (mPointCtrl) {
    const auto [l, r] = mPointCtrl->Update(pointerMode, frameInfo);
    if (Config::PointerSource == PointerSource::PointCtrl) {
      leftHand.mPose = l.mPose;
      leftHand.mDirection = l.mDirection;
      rightHand.mPose = r.mPose;
      rightHand.mDirection = r.mDirection;
    }
    if (Config::PointCtrlFCUMapping != PointCtrlFCUMapping::Disabled) {
      leftHand.mActions.mPrimary
        = leftHand.mActions.mPrimary || l.mActions.mPrimary;
      leftHand.mActions.mSecondary
        = leftHand.mActions.mSecondary || l.mActions.mSecondary;
      if (l.mActions.mValueChange != ActionState::ValueChange::None) {
        leftHand.mActions.mValueChange = l.mActions.mValueChange;
      }

      rightHand.mActions.mPrimary
        = rightHand.mActions.mPrimary || r.mActions.mPrimary;
      rightHand.mActions.mSecondary
        = rightHand.mActions.mSecondary || r.mActions.mSecondary;
      if (r.mActions.mValueChange != ActionState::ValueChange::None) {
        rightHand.mActions.mValueChange = r.mActions.mValueChange;
      }
    }
  }

  if (mHandTracking) {
    if (leftHand.mActions.Any()) {
      mHandTracking->KeepAlive(XR_HAND_LEFT_EXT, frameInfo);
    }
    if (rightHand.mActions.Any()) {
      mHandTracking->KeepAlive(XR_HAND_RIGHT_EXT, frameInfo);
    }
  }

  const InputSnapshot leftSnapshot {frameInfo, leftHand};
  const InputSnapshot rightSnapshot {frameInfo, rightHand};

  leftHand = this->SmoothHand(leftSnapshot, mPreviousFrameLeftHand);
  rightHand = this->SmoothHand(rightSnapshot, mPreviousFrameRightHand);

  if (mVirtualTouchScreen) {
    mVirtualTouchScreen->Update(leftHand, rightHand);
  }

  if (mVirtualController) {
    if (!leftHand.mPose) {
      leftHand.mPose = ProjectDirection(frameInfo, leftHand);
    }
    if (!rightHand.mPose) {
      rightHand.mPose = ProjectDirection(frameInfo, rightHand);
    }
    mVirtualController->Update(frameInfo, leftHand, rightHand);
  }

  mPreviousFrameLeftHand = leftSnapshot;
  mPreviousFrameRightHand = rightSnapshot;

  return XR_SUCCESS;
}

std::optional<XrPosef> APILayer::ProjectDirection(
  const FrameInfo& frameInfo,
  const InputState& hand) const {
  if (hand.mPose) {
    return hand.mPose;
  }

  if (!hand.mDirection) {
    return {};
  }

  const auto rx = hand.mDirection->x;
  const auto ry = hand.mDirection->y;

  const auto pointDirection
    = Quaternion::CreateFromAxisAngle(Vector3::UnitX, rx)
    * Quaternion::CreateFromAxisAngle(Vector3::UnitY, -ry);

  const auto p = Vector3::Transform(
    {0.0f, 0.0f, -Config::ProjectionDistance}, pointDirection);
  const auto o = pointDirection;

  const XrPosef viewPose {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };

  const auto worldPose = viewPose * frameInfo.mViewInLocal;
  return worldPose;
}

InputState APILayer::SmoothHand(
  const InputSnapshot& currentFrame,
  const std::optional<InputSnapshot>& maybePreviousFrame) const {
  const auto& currentInput = currentFrame.mInputState;
  if (currentInput.mPointerMode == PointerMode::None) {
    return currentInput;
  }

  if (Config::SmoothingFactor > 0.99f) {
    return currentInput;
  }

  if (!maybePreviousFrame) {
    return currentInput;
  }
  const auto& previousFrame = *maybePreviousFrame;
  const auto& previousInput = previousFrame.mInputState;

  if (currentInput.mPointerMode != previousInput.mPointerMode) {
    return currentInput;
  }

  switch (currentInput.mPointerMode) {
    case PointerMode::None:
      // TODO (C++23): std::unreachable()
      __assume(false);
    case PointerMode::Direction: {
      if (!(currentInput.mDirection && previousInput.mDirection)) {
        return currentInput;
      }

      const auto currentPose
        = this->ProjectDirection(currentFrame.mFrameInfo, currentInput);
      const auto previousPose
        = this->ProjectDirection(previousFrame.mFrameInfo, previousInput);
      if (!(currentPose && previousPose)) {
        return currentInput;
      }

      const auto p = (this->SmoothPose(*currentPose, *previousPose)
                      * currentFrame.mFrameInfo.mLocalInView)
                       .position;
      const auto rx = std::atan2f(p.y, -p.z);
      const auto ry = std::atan2f(p.x, -p.z);
      auto ret = currentInput;
      ret.mDirection = {rx, ry};
      return ret;
    }
    case PointerMode::Pose: {
      if (!(currentInput.mPose && previousInput.mPose)) {
        return currentInput;
      }

      auto ret = currentInput;
      ret.mPose = this->SmoothPose(*currentInput.mPose, *previousInput.mPose);
      return ret;
    }
    default:
      __assume(false);
  }
}

XrPosef APILayer::SmoothPose(
  const XrPosef& currentPose,
  const XrPosef& previousPose) const {
  const auto ao = XrQuatToSM(previousPose.orientation);
  const auto bo = XrQuatToSM(currentPose.orientation);
  const auto ap = XrVecToSM(previousPose.position);
  const auto bp = XrVecToSM(currentPose.position);

  return {
    SMQuatToXr(Quaternion::Slerp(ao, bo, Config::SmoothingFactor)),
    SMVecToXr(Vector3::Lerp(ap, bp, Config::SmoothingFactor)),
  };
}

}// namespace HandTrackedCockpitClicking
