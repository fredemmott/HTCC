// Copyright (c) 2025-present Frederick Emmott
// SPDX-License-Identifier: MIT
#include "Utf8.h"

namespace HandTrackedCockpitClicking::Utf8 {

std::string FromWide(const std::wstring_view in) {
  const auto byteCount = WideCharToMultiByte(
    CP_UTF8, 0, in.data(), in.size(), nullptr, 0, nullptr, nullptr);
  std::string ret(byteCount, 0);
  WideCharToMultiByte(
    CP_UTF8, 0, in.data(), in.size(), ret.data(), byteCount, nullptr, nullptr);
  return ret;
}

std::wstring ToWide(const std::string_view in) {
  const auto wideCharCount
    = MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), nullptr, 0);
  std::wstring ret(wideCharCount, 0);
  MultiByteToWideChar(
    CP_UTF8, 0, in.data(), in.size(), ret.data(), wideCharCount);
  return ret;
}

}// namespace HandTrackedCockpitClicking::Utf8