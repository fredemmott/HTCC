// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
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
