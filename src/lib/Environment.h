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

#include <cinttypes>

#define HandTrackedCockpitClicking_ENVIRONMENT_INFO \
  IT(bool, App_Enabled_XR_EXT_hand_tracking, false) \
  IT(bool, Have_XR_KHR_win32_convert_performance_counter_time, false) \
  IT(bool, Have_XR_EXT_hand_tracking, false) \
  IT(bool, Have_XR_FB_hand_tracking_aim, false) \
  IT(bool, IsPointCtrlCalibration, false)

namespace HandTrackedCockpitClicking::Environment {

void Load();

#define IT(native_type, name, default) \
  extern native_type name; \
  constexpr native_type name##Default {default};
HandTrackedCockpitClicking_ENVIRONMENT_INFO
#undef IT

}// namespace HandTrackedCockpitClicking::Environment
