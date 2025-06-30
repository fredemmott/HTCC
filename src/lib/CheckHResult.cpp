// Copyright (c) 2025-present Frederick Emmott
// SPDX-License-Identifier: MIT

#include "CheckHResult.hpp"

namespace HandTrackedCockpitClicking {

static void ThrowHResult(
  const HRESULT ret,
  const std::source_location& caller) {
  const std::error_code ec {ret, std::system_category()};

  const auto msg = std::format(
    "HRESULT failed: {:#010x} @ {} - {}:{}:{} - {}\n",
    std::bit_cast<uint32_t>(ret),
    caller.function_name(),
    caller.file_name(),
    caller.line(),
    caller.column(),
    ec.message());
  OutputDebugStringA(msg.c_str());
  throw ec;
}

void CheckHResult(const HRESULT ret, const std::source_location& caller) {
  if (SUCCEEDED(ret)) [[likely]] {
    return;
  }
  ThrowHResult(ret, caller);
}

}// namespace HandTrackedCockpitClicking