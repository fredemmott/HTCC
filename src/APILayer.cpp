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
#include "HandTrackingSource.h"
#include "OpenXRNext.h"
#include "PointCtrlSource.h"
#include "VirtualTouchScreen.h"

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace DCSQuestHandTracking {

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

APILayer::APILayer(XrSession session, const std::shared_ptr<OpenXRNext>& next)
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

  mHandTracking
    = std::make_unique<HandTrackingSource>(next, session, mViewSpace);
  mPointCtrl = std::make_unique<PointCtrlSource>();

  DebugPrint("Fully initialized.");
}

APILayer::~APILayer() {
  if (mViewSpace) {
    mOpenXR->xrDestroySpace(mViewSpace);
  }
}

XrResult APILayer::xrEndFrame(
  XrSession session,
  const XrFrameEndInfo* frameEndInfo) {
  if (!mVirtualTouchScreen) [[unlikely]] {
    mVirtualTouchScreen = std::make_unique<VirtualTouchScreen>(
      mOpenXR, session, frameEndInfo->displayTime, mViewSpace);
  }

  ActionState actionState {};
  std::optional<XrVector2f> rotation;

  if (mHandTracking) {
    mHandTracking->Update(frameEndInfo->displayTime);
    actionState = mHandTracking->GetActionState();
    if (Config::PointerSource == PointerSource::OculusHandTracking) {
      rotation = mHandTracking->GetRXRY();
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
    }
  }

  mVirtualTouchScreen->Update(rotation, actionState);

  return mOpenXR->xrEndFrame(session, frameEndInfo);
}

}// namespace DCSQuestHandTracking
