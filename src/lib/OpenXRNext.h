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
#include <openxr/openxr_platform.h>

#include <utility>

#include "DebugPrint.h"

#define INTERCEPTED_OPENXR_FUNCS \
  IT(xrEnumerateApiLayerProperties) \
  IT(xrEnumerateInstanceExtensionProperties) \
  IT(xrGetSystemProperties) \
  IT(xrDestroyInstance) \
  IT(xrCreateSession) \
  IT(xrDestroySession) \
  IT(xrLocateSpace) \
  IT(xrWaitFrame) \
  IT(xrSuggestInteractionProfileBindings) \
  IT(xrCreateAction) \
  IT(xrCreateActionSpace) \
  IT(xrGetActionStateBoolean) \
  IT(xrGetActionStateFloat) \
  IT(xrGetActionStatePose) \
  IT(xrSyncActions) \
  IT(xrGetCurrentInteractionProfile) \
  IT(xrPollEvent) \
  IT_EXT(XR_EXT_hand_tracking, xrCreateHandTrackerEXT)
#define NEXT_OPENXR_FUNCS \
  INTERCEPTED_OPENXR_FUNCS \
  IT(xrCreateReferenceSpace) \
  IT(xrDestroySpace) \
  IT(xrLocateViews) \
  IT(xrPathToString) \
  IT(xrGetInstanceProperties) \
  IT_EXT(XR_KHR_win32_convert_performance_counter_time, xrConvertTimeToWin32PerformanceCounterKHR) \
  IT_EXT(XR_KHR_win32_convert_performance_counter_time, xrConvertWin32PerformanceCounterToTimeKHR) \
  IT_EXT(XR_EXT_hand_tracking, xrDestroyHandTrackerEXT) \
  IT_EXT(XR_EXT_hand_tracking, xrLocateHandJointsEXT)

namespace HandTrackedCockpitClicking {

class OpenXRNext final {
 public:
  OpenXRNext(XrInstance, PFN_xrGetInstanceProcAddr);

#define IT_EXT(ext, func) IT(func)
#define IT(func) \
  template <class... Args> \
  auto raw_##func(Args&&... args) { \
    if (!this->m_##func) [[unlikely]] { \
      auto nextResult = this->m_xrGetInstanceProcAddr( \
        mInstance, \
        #func, \
        reinterpret_cast<PFN_xrVoidFunction*>(&this->m_##func)); \
      if (nextResult != XR_SUCCESS) { \
        DebugPrint("Failed to find function {}: {}", #func, (int)nextResult); \
        return nextResult; \
      } \
    } \
    return this->m_##func(std::forward<Args>(args)...); \
  } \
  template <class... Args> \
  auto func(Args&&... args) { \
    return raw_##func(std::forward<Args>(args)...); \
  } \
  template <class... Args> \
  bool check_##func(Args&&... args) { \
    return this->func(std::forward<Args>(args)...) == XR_SUCCESS; \
  } \
  inline bool have_##func() const noexcept { \
    return m_##func != nullptr; \
  }
  IT(xrGetInstanceProcAddr)
  NEXT_OPENXR_FUNCS
#undef IT
#undef IT_EXT

 private:
  XrInstance mInstance;
#define IT_EXT(ext, func) IT(func)
#define IT(func) PFN_##func m_##func {nullptr};
  IT(xrGetInstanceProcAddr)
  NEXT_OPENXR_FUNCS
#undef IT
#undef IT_EXT
};

}// namespace HandTrackedCockpitClicking
