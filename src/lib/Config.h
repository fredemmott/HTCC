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
#include <numbers>

namespace HandTrackedCockpitClicking {
enum class PointerSource : DWORD {
  OpenXRHandTracking = 0,
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
  Disabled = 0,
  Classic = 1,
  Modal = 2,
  ModalWithLeftLock = 3,
};
enum class HandTrackingOrientation : DWORD {
  Raw = 0,
  RayCast = 1,
};
enum class VRControllerActionSinkMapping : DWORD {
  DCS = 0,
  MSFS = 1,
};
enum class VRControllerPointerSinkWorldLock : DWORD {
  Nothing = 0,
  Orientation = 1,
  OrientationAndSoftPosition = 2,
};
}// namespace HandTrackedCockpitClicking

#define HandTrackedCockpitClicking_DWORD_SETTINGS \
  IT(bool, Enabled, false) \
  IT(uint8_t, VerboseDebug, 0) \
  IT(uint8_t, MirrorEye, 1) \
  IT(bool, EnableFBOpenXRExtensions, true) \
  IT(bool, OneHandOnly, false) \
  IT(bool, UseHandTrackingAimPointFB, true) \
  IT(XrHandJointEXT, HandTrackingAimJoint, XR_HAND_JOINT_INDEX_PROXIMAL_EXT) \
  IT(bool, PinchToClick, true) \
  IT(bool, PinchToScroll, true) \
  IT(uint16_t, ShortPressLongPressMilliseconds, 200) \
  IT(uint16_t, ScrollWheelMilliseconds, 500) \
  IT(uint16_t, ScrollAccelerationMilliseconds, 3000) \
  IT(uint32_t, HandTrackingWakeMilliseconds, 100) \
  IT(uint32_t, HandTrackingSleepMilliseconds, 2000) \
  IT(uint32_t, HandTrackingActionMilliseconds, 30) \
  IT(bool, PointCtrlSupportHotplug, true) \
  IT(uint16_t, PointCtrlCenterX, 32767) \
  IT(uint16_t, PointCtrlCenterY, 32767) \
  IT( \
    HandTrackedCockpitClicking::PointCtrlFCUMapping, \
    PointCtrlFCUMapping, \
    HandTrackedCockpitClicking::PointCtrlFCUMapping::Classic) \
  IT( \
    HandTrackedCockpitClicking::PointerSource, \
    PointerSource, \
    HandTrackedCockpitClicking::PointerSource::OpenXRHandTracking) \
  IT( \
    HandTrackedCockpitClicking::PointerSink, \
    PointerSink, \
    HandTrackedCockpitClicking::PointerSink::VirtualVRController) \
  IT( \
    HandTrackedCockpitClicking::ActionSink, \
    ClickActionSink, \
    HandTrackedCockpitClicking::ActionSink::MatchPointerSink) \
  IT( \
    HandTrackedCockpitClicking::ActionSink, \
    ScrollActionSink, \
    HandTrackedCockpitClicking::ActionSink::MatchPointerSink) \
  IT( \
    HandTrackedCockpitClicking::HandTrackingOrientation, \
    HandTrackingOrientation, \
    HandTrackedCockpitClicking::HandTrackingOrientation::RayCast) \
  IT( \
    HandTrackedCockpitClicking::VRControllerActionSinkMapping, \
    VRControllerActionSinkMapping, \
    HandTrackedCockpitClicking::VRControllerActionSinkMapping::DCS) \
  IT( \
    HandTrackedCockpitClicking::VRControllerPointerSinkWorldLock, \
    VRControllerPointerSinkWorldLock, \
    HandTrackedCockpitClicking::VRControllerPointerSinkWorldLock:: \
      OrientationAndSoftPosition) \
  IT(uint16_t, PointCtrlVID, 0x04d8) \
  IT(uint16_t, PointCtrlPID, 0xeeec) \
  IT(uint8_t, PointCtrlFCUButtonL1, 0) \
  IT(uint8_t, PointCtrlFCUButtonL2, 1) \
  IT(uint8_t, PointCtrlFCUButtonL3, 2) \
  IT(uint8_t, PointCtrlFCUButtonR1, 3) \
  IT(uint8_t, PointCtrlFCUButtonR2, 4) \
  IT(uint8_t, PointCtrlFCUButtonR3, 5) \
  IT(uint32_t, PointCtrlSleepMilliseconds, 20000)

#define HandTrackedCockpitClicking_FLOAT_SETTINGS \
  IT(PointCtrlRadiansPerUnitX, 3.009e-5f) \
  IT(PointCtrlRadiansPerUnitY, 3.009e-5f) \
  IT(PointCtrlProjectionDistance, 0.3f) \
  IT(VRVerticalOffset, -0.04f) \
  IT(VRFarDistance, 0.8f) \
  IT(VRControllerActionSinkSecondsPerRotation, 4.0f) \
  IT(VRControllerPointerSinkSoftWorldLockDistance, 0.05f) \
  IT(HandTrackingSleepSpeed, 0.1f) \
  IT(HandTrackingWakeSpeed, 0.5f) \
  IT(HandTrackingActionSpeed, 0.08f) \
  IT(HandTrackingWakeVFOV, std::numbers::pi_v<float> / 6) \
  IT(HandTrackingWakeHFOV, std::numbers::pi_v<float> / 3) \
  IT(HandTrackingActionVFOV, std::numbers::pi_v<float> / 6) \
  IT(HandTrackingActionHFOV, std::numbers::pi_v<float> / 3)

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

  inline bool
  IsRaycastOrientation() {
  return (
    (Config::PointerSource == PointerSource::PointCtrl)
    || Config::HandTrackingOrientation == HandTrackingOrientation::RayCast);
}

void Load();
void Save();

}// namespace HandTrackedCockpitClicking::Config
