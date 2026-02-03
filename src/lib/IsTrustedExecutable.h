// Copyright 2026 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>

namespace HandTrackedCockpitClicking {

[[nodiscard]]
bool IsTrustedExecutable(const std::filesystem::path&);

}