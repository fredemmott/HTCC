// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>

#include <format>
#include <version>

namespace HandTrackedCockpitClicking {

TRACELOGGING_DECLARE_PROVIDER(gTraceProvider);

namespace detail {
void DebugPrintString(std::wstring_view);
}

template <class... Args>
void DebugPrint(std::wformat_string<Args...> fmt, Args&&... args) {
  detail::DebugPrintString(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void DebugPrint(std::format_string<Args...> fmt, Args&&... args) {
  detail::DebugPrintString(
    winrt::to_hstring(std::format(fmt, std::forward<Args>(args)...)));
}

}// namespace HandTrackedCockpitClicking
