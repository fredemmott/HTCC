// Copyright (c) 2025-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <source_location>

#include "pch.h"

namespace HandTrackedCockpitClicking {

void CheckHResult(const HRESULT ret, const std::source_location& loc = {});

}// namespace HandTrackedCockpitClicking