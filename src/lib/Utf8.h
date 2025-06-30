// Copyright (c) 2025-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace HandTrackedCockpitClicking::Utf8 {

std::string FromWide(std::wstring_view);
std::wstring ToWide(std::string_view);

}// namespace HandTrackedCockpitClicking::Utf8