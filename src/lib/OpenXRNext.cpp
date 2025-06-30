// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#include "OpenXRNext.h"

namespace HandTrackedCockpitClicking {

OpenXRNext::OpenXRNext(XrInstance instance, PFN_xrGetInstanceProcAddr getNext)
  :
#define IT_EXT(ext, func) IT(func)
#define IT(func) func(instance, getNext),
NEXT_OPENXR_FUNCS
#undef IT_EXT
#undef IT
xrGetInstanceProcAddr(getNext)
{
}

}// namespace HandTrackedCockpitClicking
