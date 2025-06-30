// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <cinttypes>
#include <string_view>

namespace HTCCSettings::Version {

extern const std::string_view ReleaseName;
extern const std::string_view BuildMode;

extern const uint16_t Major;
extern const uint16_t Minor;
extern const uint16_t Patch;
extern const uint16_t Build;

extern const bool IsGitHubActionsBuild;

}// namespace HTCCSettings::Version
