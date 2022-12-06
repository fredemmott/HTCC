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
#include "FrameInfo.h"

#include <Windows.h>

#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

class OpenXRNext;

FrameInfo::FrameInfo(
  OpenXRNext* openXR,
  XrInstance instance,
  XrSpace localSpace,
  XrSpace viewSpace,
  XrTime predictedDisplayTime)
  : mPredictedDisplayTime(predictedDisplayTime) {
  LARGE_INTEGER nowPC;
  QueryPerformanceCounter(&nowPC);
  openXR->xrConvertWin32PerformanceCounterToTimeKHR(instance, &nowPC, &mNow);

  XrSpaceLocation location {XR_TYPE_SPACE_LOCATION};
  if (openXR->check_xrLocateSpace(
        localSpace, viewSpace, predictedDisplayTime, &location)) {
    mLocalInView = location.pose;
  }
  if (openXR->check_xrLocateSpace(
        viewSpace, localSpace, predictedDisplayTime, &location)) {
    mViewInLocal = location.pose;
  }
}

}// namespace HandTrackedCockpitClicking
