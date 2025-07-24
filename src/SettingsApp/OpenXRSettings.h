// Copyright (c) 2024-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once
#include <wil/registry.h>

/** Class encapsulating actual OpenXR settings, rather than HTCC settings.
 *
 * For example, this includes runtime and API layer status.
 */
struct OpenXRSettings {
  enum class UltraleapStatus {
    NotFound,
    HTCCFirst,
    UltraleapFirst,
  };

  OpenXRSettings();
  ~OpenXRSettings();
  static constexpr wchar_t OpenXRSubkey[] = L"SOFTWARE\\Khronos\\OpenXR\\1";
  static constexpr wchar_t APILayerSubkey[]
    = L"SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\Implicit";

  std::wstring GetApiLayerPath() const noexcept {
    return mAPILayerPath;
  }

  bool IsApiLayerEnabled() const noexcept {
    return mData.mIsApiLayerEnabled;
  }

  bool HaveOpenXR() const noexcept {
    return mData.mHaveOpenXR;
  }

  bool HaveHandTracking() const noexcept {
    return mData.mHaveHandTracking;
  }

  bool HaveHandTrackingAimPointFB() const noexcept {
    return mData.mHaveHandTrackingAimPointFB;
  }

  UltraleapStatus GetUltraleapLayerStatus() const noexcept {
    return mData.mUltraleapStatus;
  }

  std::wstring GetUltraleapLayerPath() const noexcept {
    return mData.mUltraleapPath;
  }

 private:
  struct MutableData {
    bool mIsApiLayerEnabled {false};
    bool mHaveOpenXR {false};
    bool mHaveHandTracking {false};
    bool mHaveHandTrackingAimPointFB {false};

    UltraleapStatus mUltraleapStatus {UltraleapStatus::NotFound};
    std::wstring mUltraleapPath;
  };

  const std::wstring mAPILayerPath;
  MutableData mData;

  wil::unique_registry_watcher mRegWatcher;

  void Load();
  void LoadRuntime();
  void LoadUltraleap();
};