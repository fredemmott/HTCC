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

#include <loader_interfaces.h>
#include <openxr/openxr.h>

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

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace HandTrackedCockpitClicking {

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

APILayer::APILayer(
  XrInstance instance,
  XrSession session,
  const std::shared_ptr<OpenXRNext>& next)
  : mOpenXR(next) {
  DebugPrint("{}()", __FUNCTION__);
  auto oxr = next.get();

  XrReferenceSpaceCreateInfo referenceSpace {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .next = nullptr,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
    .poseInReferenceSpace = XR_POSEF_IDENTITY,
  };

  auto nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mViewSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create view space: {}", nextResult);
    return;
  }

  if (Environment::Have_XR_EXT_HandTracking) {
    mHandTracking
      = std::make_unique<HandTrackingSource>(next, session, mViewSpace);
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

XrResult APILayer::xrCreateActionSpace(
  XrSession session,
  const XrActionSpaceCreateInfo* createInfo,
  XrSpace* space) {
  if (mVirtualController) {
    return mVirtualController->xrCreateActionSpace(session, createInfo, space);
  }
  return mOpenXR->xrCreateActionSpace(session, createInfo, space);
}

XrResult APILayer::xrWaitFrame(
  XrSession session,
  const XrFrameWaitInfo* frameWaitInfo,
  XrFrameState* state) {
  const auto nextResult = mOpenXR->xrWaitFrame(session, frameWaitInfo, state);
  if (nextResult != XR_SUCCESS) {
    return nextResult;
  }

  if (
    (!mVirtualTouchScreen)
    && (VirtualTouchScreenSink::IsActionSink() || VirtualTouchScreenSink::IsPointerSink())) {
    mVirtualTouchScreen = std::make_unique<VirtualTouchScreenSink>(
      mOpenXR, session, state->predictedDisplayTime, mViewSpace);
  }

  ActionState actionState {};
  std::optional<XrVector2f> rotation;
  std::optional<XrPosef> leftAimPose;
  std::optional<XrPosef> rightAimPose;

  if (mHandTracking) {
    mHandTracking->Update(state->predictedDisplayTime);
    actionState = mHandTracking->GetActionState();
    if (Config::PointerSource == PointerSource::OculusHandTracking) {
      rotation = mHandTracking->GetRXRY();
      auto [left, right] = mHandTracking->GetPoses();
      leftAimPose = left;
      rightAimPose = right;
    }
  }

  if (mPointCtrl) {
    mPointCtrl->Update();
    if (Config::PointCtrlFCUClicks) {
      const auto as = mPointCtrl->GetActionState();
      actionState = {
        .mLeftClick = actionState.mLeftClick || as.mLeftClick,
        .mRightClick = actionState.mRightClick || as.mRightClick,
        .mWheelUp = actionState.mWheelUp || as.mWheelUp,
        .mWheelDown = actionState.mWheelDown || as.mWheelDown,
      };
    }
    if (Config::PointerSource == PointerSource::PointCtrl) {
      rotation = mPointCtrl->GetRXRY();
      auto [left, right] = mPointCtrl->GetPoses();
      leftAimPose = left;
      rightAimPose = right;
    }
  }

  if (actionState.mWheelUp && actionState.mWheelDown) {
    actionState.mWheelUp = false;
    actionState.mWheelDown = false;
  }

  if (mVirtualTouchScreen) {
    mVirtualTouchScreen->Update(rotation, actionState);
  }

  if (mVirtualController) {
    mVirtualController->Update(leftAimPose, rightAimPose, actionState);
  }

  return XR_SUCCESS;
}

}// namespace HandTrackedCockpitClicking
