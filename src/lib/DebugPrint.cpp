// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT

#include "DebugPrint.h"

#include <TraceLoggingActivity.h>
#include <debugapi.h>
#include <evntrace.h>
#include <winmeta.h>

namespace HandTrackedCockpitClicking {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("FredEmmott.HandTrackedCockpitClicking").guid
 * d9675adc-8f15-5a67-f177-7b6ee279ae95
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "FredEmmott.HandTrackedCockpitClicking",
  (0xd9675adc, 0x8f15, 0x5a67, 0xf1, 0x77, 0x7b, 0x6e, 0xe2, 0x79, 0xae, 0x95));

}// namespace HandTrackedCockpitClicking

namespace HandTrackedCockpitClicking::detail {

void DebugPrintString(std::wstring_view message) {
  TraceLoggingWrite(
    gTraceProvider,
    "DebugPrint",
    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
    TraceLoggingCountedWideString(message.data(), message.size(), "msg"));
  const auto formatted
    = std::format(L"[HandTrackedCockpitClicking] {}\n", message);

  OutputDebugStringW(formatted.c_str());
}

}// namespace HandTrackedCockpitClicking::detail
