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

#include <tuple>

#include "InputSource.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class HandTrackingSource final : public InputSource {
 public:
  HandTrackingSource(
    const std::shared_ptr<OpenXRNext>& next,
    XrInstance instance,
    XrSession session,
    XrSpace viewSpace,
    XrSpace localSpace);
  ~HandTrackingSource();

  std::tuple<InputState, InputState> Update(PointerMode, const FrameInfo&)
    override;

  void KeepAlive(XrHandEXT, const FrameInfo&);

 private:
  std::shared_ptr<OpenXRNext> mOpenXR;
  XrInstance mInstance {};
  XrSession mSession {};
  XrSpace mViewSpace {};
  XrSpace mLocalSpace {};

  struct Hand {
    XrHandEXT mHand;
    InputState mState {mHand};
    XrHandTrackerEXT mTracker {};
    bool mSleeping {false};
    XrTime mLastKeepAliveAt {};
    XrTime mLastSleepSpeedAt {};
  };

  Hand mLeftHand {XR_HAND_LEFT_EXT};
  Hand mRightHand {XR_HAND_RIGHT_EXT};

  void InitHandTracker(Hand* hand);
  void UpdateHand(const FrameInfo&, Hand* hand);
  std::tuple<XrPosef, XrVector2f> RaycastPose(
    const FrameInfo&,
    const XrPosef& pose);
};

}// namespace HandTrackedCockpitClicking
