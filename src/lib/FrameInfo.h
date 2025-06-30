// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include "openxr.h"

namespace HandTrackedCockpitClicking {

class OpenXRNext;

struct FrameInfo {
  FrameInfo() = default;
  FrameInfo(
    OpenXRNext* next,
    XrInstance instance,
    XrSpace localSpace,
    XrSpace viewSpace,
    XrTime predictedDisplayTime);

  XrTime mNow {};
  XrTime mPredictedDisplayTime {};
  XrPosef mLocalInView {XR_POSEF_IDENTITY};
  XrPosef mViewInLocal {XR_POSEF_IDENTITY};
};

}// namespace HandTrackedCockpitClicking
