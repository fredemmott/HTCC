// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <cinttypes>

#define HandTrackedCockpitClicking_ENVIRONMENT_INFO \
  IT(bool, App_Enabled_XR_EXT_hand_tracking, false) \
  IT(bool, App_Enabled_XR_KHR_win32_convert_performance_counter_time, false) \
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
