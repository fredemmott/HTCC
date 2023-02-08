/*
 * MIT License
 *
 * Copyright (c) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
