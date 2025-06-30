// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#include "Environment.h"

#include "Config.h"
#include "DebugPrint.h"

namespace HandTrackedCockpitClicking::Environment {

void Load() {
}

#define IT(native_type, name, default) native_type name {default};
HandTrackedCockpitClicking_ENVIRONMENT_INFO
#undef IT

}// namespace HandTrackedCockpitClicking::Environment
