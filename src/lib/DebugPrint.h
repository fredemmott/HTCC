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
#pragma once

#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>

#include <format>
#include <version>

namespace HandTrackedCockpitClicking {

TRACELOGGING_DECLARE_PROVIDER(gTraceProvider);

namespace detail {
void DebugPrintString(std::wstring_view);

#if __cpp_lib_format >= 202207L
// Standard
template <class... T>
using format_string = std::format_string<T...>;
template <class... T>
using wformat_string = std::wformat_string<T...>;
#else
// MSVC-specific, but removed in VS 2022 v17.5
template <class... T>
using format_string = std::_Fmt_string<T...>;
template <class... T>
using wformat_string = std::_Fmt_wstring<T...>;
#endif

}// namespace detail

template <class... Args>
void DebugPrint(detail::wformat_string<Args...> fmt, Args&&... args) {
  detail::DebugPrintString(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void DebugPrint(detail::format_string<Args...> fmt, Args&&... args) {
  detail::DebugPrintString(
    winrt::to_hstring(std::format(fmt, std::forward<Args>(args)...)));
}

}// namespace HandTrackedCockpitClicking
