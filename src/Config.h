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

namespace DCSQuestHandTracking {
enum class PointerSource : DWORD {
  OculusHandTracking = 0,
  PointCtrl = 1,
};
enum class PointerSink : DWORD {
  VirtualTouchScreen = 0,
  VirtualVRController = 1,
};
}// namespace DCSQuestHandTracking

#define DCSQUESTHANDTRACKING_DWORD_SETTINGS \
  IT(bool, Enabled, true) \
  IT(bool, CheckDCS, true) \
  IT(uint8_t, VerboseDebug, 0) \
  IT(uint8_t, MirrorEye, 1) \
  IT(bool, PinchToClick, true) \
  IT(bool, PinchToScroll, true) \
  IT(bool, PointCtrlFCUClicks, true) \
  IT(uint16_t, PointCtrlCenterX, 32767) \
  IT(uint16_t, PointCtrlCenterY, 32767) \
  IT( \
    DCSQuestHandTracking::PointerSource, \
    PointerSource, \
    DCSQuestHandTracking::PointerSource::OculusHandTracking) \
  IT( \
    DCSQuestHandTracking::PointerSink, \
    PointerSink, \
    DCSQuestHandTracking::PointerSink::VirtualTouchScreen)
#define DCSQUESTHANDTRACKING_FLOAT_SETTINGS \
  IT(PointCtrlRadiansPerUnitX, 3.009e-5f) \
  IT(PointCtrlRadiansPerUnitY, 3.009e-5f) \
  IT(PointCtrlProjectionDistance, 0.60f)

namespace DCSQuestHandTracking::Config {

#define IT(native_type, name, default) \
  extern native_type name; \
  constexpr native_type name##Default {default};
DCSQUESTHANDTRACKING_DWORD_SETTINGS
#undef IT
#define IT(name, default) extern float name; \
constexpr float name##Default {default};
DCSQUESTHANDTRACKING_FLOAT_SETTINGS
#undef IT

void Load();
void Save();

}// namespace DCSQuestHandTracking::Config
