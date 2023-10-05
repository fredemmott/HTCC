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
#include "Environment.h"

#include "Config.h"
#include "DebugPrint.h"

namespace HandTrackedCockpitClicking::Environment {

void Load() {
  if (Config::ForceHaveXRExtHandTracking) {
    DebugPrint("Force-enabling XR_EXT_hand_tracking due to config");
    Have_XR_EXT_HandTracking = true;
  }
  if (Config::ForceHaveXRFBHandTrackingAim) {
    DebugPrint("Force-enabling XR_FB_hand_tracking_aim due to config");
    Have_XR_FB_HandTracking_Aim = true;
  }
}

#define IT(native_type, name, default) native_type name {default};
HandTrackedCockpitClicking_ENVIRONMENT_INFO
#undef IT

}// namespace HandTrackedCockpitClicking::Environment
