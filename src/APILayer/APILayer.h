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

#include <memory>

#include "FrameInfo.h"
#include "InputState.h"

namespace HandTrackedCockpitClicking {

class HandTrackingSource;
class OpenXRNext;
class PointCtrlSource;
class VirtualControllerSink;
class VirtualTouchScreenSink;
struct FrameInfo;

class APILayer final {
 public:
  APILayer() = delete;
  APILayer(XrInstance, XrSession, const std::shared_ptr<OpenXRNext>&);
  virtual ~APILayer();

  XrResult xrWaitFrame(
    XrSession session,
    const XrFrameWaitInfo* frameWaitInfo,
    XrFrameState* state);

  XrResult xrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings);

  XrResult xrGetCurrentInteractionProfile(
    XrSession session,
    XrPath topLevelUserPath,
    XrInteractionProfileState* interactionProfile);

  XrResult xrCreateAction(
    XrActionSet actionSet,
    const XrActionCreateInfo* createInfo,
    XrAction* action);

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

 private:
  std::optional<XrPosef> ProjectDirection(
    const FrameInfo& frameInfo,
    const InputState& hand) const;

  struct InputSnapshot {
    FrameInfo mFrameInfo {};
    InputState mInputState {};
  };

  InputState SmoothHand(
    const InputSnapshot& currentFrame,
    const std::optional<InputSnapshot>& previousFrame) const;
  XrPosef SmoothPose(const XrPosef& current, const XrPosef& previous) const;

  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};

  std::unique_ptr<HandTrackingSource> mHandTracking;
  std::unique_ptr<PointCtrlSource> mPointCtrl;
  std::unique_ptr<VirtualTouchScreenSink> mVirtualTouchScreen;
  std::unique_ptr<VirtualControllerSink> mVirtualController;

  std::optional<InputSnapshot> mPreviousFrameLeftHand;
  std::optional<InputSnapshot> mPreviousFrameRightHand;
};

}// namespace HandTrackedCockpitClicking
