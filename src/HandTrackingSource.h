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

#include "ActionState.h"
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class HandTrackingSource final {
 public:
  HandTrackingSource(
    const std::shared_ptr<OpenXRNext>& next,
    XrSession session,
    XrSpace space);
  ~HandTrackingSource();

  void Update(XrTime displayTime);

  std::tuple<std::optional<XrPosef>, std::optional<XrPosef>> GetPoses() const;
  /** Rotation around the X axis, and rotation around the y Axis.
   *
   * This can be a bit counter-intuitive when mapping to the screen:
   * - left-right movement is along the X axis, so it is rotation around the Y
   * axis
   * - up-down movement is along the Y axis, so it is rotation around the X axis
   */
  std::optional<XrVector2f> GetRXRY() const;
  ActionState GetActionState() const;

 private:
  void InitHandTrackers();

  std::shared_ptr<OpenXRNext> mOpenXR;
  XrSession mSession;
  XrSpace mSpace;

  XrHandTrackerEXT mLeftHand {};
  XrHandTrackerEXT mRightHand {};

  std::optional<XrPosef> mLeftHandPose;
  std::optional<XrPosef> mRightHandPose;
  std::chrono::steady_clock::time_point mLeftHandUpdateAt {};
  std::chrono::steady_clock::time_point mRightHandUpdateAt {};

  ActionState mActionState {};
};

}// namespace HandTrackedCockpitClicking
