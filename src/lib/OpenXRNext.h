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
  IT(xrGetSystemProperties) \
  IT(xrCreateSession) \
  IT(xrDestroySession) \
  IT(xrLocateSpace) \
  IT(xrWaitFrame) \
  IT(xrSuggestInteractionProfileBindings) \
  IT(xrAttachSessionActionSets) \
  IT(xrCreateAction) \
  IT(xrCreateActionSpace) \
  IT(xrGetActionStateBoolean) \
  IT(xrGetActionStateFloat) \
  IT(xrGetActionStatePose) \
  IT(xrSyncActions) \
  IT(xrGetCurrentInteractionProfile) \
  IT(xrPollEvent) \
  IT_EXT(XR_EXT_hand_tracking, xrCreateHandTrackerEXT)
#define SPECIAL_INTERCEPTED_OPENXR_FUNCS \
  IT(xrEnumerateApiLayerProperties) \
  IT(xrEnumerateInstanceExtensionProperties) \
  IT(xrDestroyInstance)
#define NEXT_OPENXR_FUNCS \
  INTERCEPTED_OPENXR_FUNCS \
  SPECIAL_INTERCEPTED_OPENXR_FUNCS \
  IT(xrCreateReferenceSpace) \
  IT(xrDestroySpace) \
  IT(xrLocateViews) \
  IT(xrPathToString) \
  IT(xrGetInstanceProperties) \
  IT_EXT( \
    XR_KHR_win32_convert_performance_counter_time, \
    xrConvertTimeToWin32PerformanceCounterKHR) \
  IT_EXT( \
    XR_KHR_win32_convert_performance_counter_time, \
    xrConvertWin32PerformanceCounterToTimeKHR) \
  IT_EXT(XR_EXT_hand_tracking, xrDestroyHandTrackerEXT) \
  IT_EXT(XR_EXT_hand_tracking, xrLocateHandJointsEXT)

template <class T>
struct XRFuncName;
#define IT_EXT(ext, func) IT(func)
#define IT(func) \
  template <> \
  struct XRFuncName<PFN_##func> { \
    static constexpr auto value = #func; \
  };
NEXT_OPENXR_FUNCS
#undef IT_EXT
#undef IT

namespace HandTrackedCockpitClicking {

class OpenXRNext final {
 public:
  OpenXRNext(XrInstance, PFN_xrGetInstanceProcAddr);

  template <class F, class TName>
  struct Fun;

  template <class TRet, class... TArgs, class TName>
  struct Fun<TRet (*)(TArgs...), TName> {
    Fun() = delete;
    Fun(const Fun&) = delete;
    Fun(Fun&&) = delete;
    Fun& operator=(const Fun&) = delete;
    Fun& operator=(Fun&&) = delete;

    Fun(XrInstance instance, PFN_xrGetInstanceProcAddr get)
      : mInstance(instance), mGet(get) {
      [[maybe_unused]] auto loaded = Init();
    }

    TRet operator()(TArgs... args) noexcept {
      if (!mNext) {
        if (!Init()) {
          return XR_ERROR_FUNCTION_UNSUPPORTED;
        }
      }
      return std::invoke(mNext, args...);
    }

   private:
    XrInstance mInstance {};
    PFN_xrGetInstanceProcAddr mGet {};

    using FunPtr = TRet (*)(TArgs...);
    FunPtr mNext {};

    [[nodiscard]] bool Init() {
      if (mNext) {
        return true;
      }
      mGet(
        mInstance, TName::value, reinterpret_cast<PFN_xrVoidFunction*>(&mNext));
      return static_cast<bool>(mNext);
    }
  };

#define IT_EXT(ext, func) IT(func)
#define IT(func) \
  Fun<PFN_##func, XRFuncName<PFN_##func>> func; \
  template <class... Args> \
  [[nodiscard]] \
  bool check_##func(Args&&... args) { \
    return XR_SUCCEEDED(this->func(std::forward<Args>(args)...)); \
  }
  NEXT_OPENXR_FUNCS
#undef IT
#undef IT_EXT

  // Last to avoid trailing comma issues with macro expansion
  PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr {};
};

}// namespace HandTrackedCockpitClicking
