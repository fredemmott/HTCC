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

#include <cinttypes>

namespace HandTrackedCockpitClicking {
enum class PointerSource : DWORD {
  OculusHandTracking = 0,
  PointCtrl = 1,
};
enum class PointerSink : DWORD {
  VirtualTouchScreen = 0,
  VirtualVRController = 1,
};
enum class ActionSink : DWORD {
  MatchPointerSink = 0,
  VirtualTouchScreen = 1,
  VirtualVRController = 2,
};
enum class PointCtrlFCUMapping : DWORD {
  Classic = 0,
  Modal = 1,
};
}// namespace HandTrackedCockpitClicking

#define HandTrackedCockpitClicking_DWORD_SETTINGS \
  IT(bool, Enabled, false) \
  IT(uint8_t, VerboseDebug, 0) \
  IT(uint8_t, MirrorEye, 1) \
  IT(bool, EnableFBOpenXRExtensions, true) \
  IT(bool, OneHandOnly, true) \
  IT(bool, UseHandTrackingAimPointFB, false) \
  IT(XrHandJointEXT, HandTrackingAimJoint, XR_HAND_JOINT_INDEX_PROXIMAL_EXT) \
  IT(bool, RaycastHandTrackingPose, true) \
  IT(bool, PinchToClick, true) \
  IT(bool, PinchToScroll, true) \
  IT(bool, PointCtrlFCUClicks, true) \
  IT(uint16_t, PointCtrlCenterX, 32767) \
  IT(uint16_t, PointCtrlCenterY, 32767) \
  IT( \
    HandTrackedCockpitClicking::PointCtrlFCUMapping, \
    PointCtrlFCUMapping, \
    HandTrackedCockpitClicking::PointCtrlFCUMapping::Classic) \
  IT( \
    HandTrackedCockpitClicking::PointerSource, \
    PointerSource, \
    HandTrackedCockpitClicking::PointerSource::OculusHandTracking) \
  IT( \
    HandTrackedCockpitClicking::PointerSink, \
    PointerSink, \
    HandTrackedCockpitClicking::PointerSink::VirtualTouchScreen) \
  IT( \
    HandTrackedCockpitClicking::ActionSink, \
    ActionSink, \
    HandTrackedCockpitClicking::ActionSink::MatchPointerSink) \
  IT(uint16_t, PointCtrlVID, 0x04d8) \
  IT(uint16_t, PointCtrlPID, 0xeeec) \
  IT(uint8_t, PointCtrlFCUButtonL1, 0) \
  IT(uint8_t, PointCtrlFCUButtonL2, 1) \
  IT(uint8_t, PointCtrlFCUButtonL3, 2) \
  IT(uint8_t, PointCtrlFCUButtonR1, 3) \
  IT(uint8_t, PointCtrlFCUButtonR2, 4) \
  IT(uint8_t, PointCtrlFCUButtonR3, 5)

#define HandTrackedCockpitClicking_FLOAT_SETTINGS \
  IT(PointCtrlRadiansPerUnitX, 3.009e-5f) \
  IT(PointCtrlRadiansPerUnitY, 3.009e-5f) \
  IT(PointCtrlProjectionDistance, 0.60f)

#define HandTrackedCockpitClicking_STRING_SETTINGS \
  IT( \
    VirtualControllerInteractionProfilePath, \
    "/interaction_profiles/oculus/touch_controller")

namespace HandTrackedCockpitClicking::Config {

#define IT(native_type, name, default) \
  extern native_type name; \
  constexpr native_type name##Default {default};
HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, default) \
  extern float name; \
  constexpr float name##Default {default};
  HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
#define IT(name, default) \
  extern std::string name; \
  constexpr std::string_view name##Default {default};
    HandTrackedCockpitClicking_STRING_SETTINGS
#undef IT

  void
  Load();
void Save();

}// namespace HandTrackedCockpitClicking::Config
