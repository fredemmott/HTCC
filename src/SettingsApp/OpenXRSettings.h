// Copyright (c) 2024-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once
#include <wil/registry.h>

#include <shared_mutex>

/** Class encapsulating actual OpenXR settings, rather than HTCC settings.
 *
 * For example, this includes runtime and API layer status.
 */
struct OpenXRSettings {
  enum class UltraleapStatus {
    NotFound,
    HTCCFirst,
    UltraleapFirst,
    DisabledInRegistry,
    DisabledByEnvironmentVariable,
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

  bool HaveHandTrackingAimFB() const noexcept {
    return mData.mHaveHandTrackingAimFB;
  }

  UltraleapStatus GetUltraleapLayerStatus() const noexcept {
    return mData.mUltraleapStatus;
  }

  std::wstring GetUltraleapLayerPath() const noexcept {
    return mData.mUltraleapPath;
  }

  template <std::convertible_to<std::function<void()>> F>
  void OnReload(F&& onReload) {
    const auto lock = std::unique_lock(mMutex);
    mOnReload.emplace_back(std::forward<F>(onReload));
  }

  void lock_shared() {
    mMutex.lock_shared();
  }

  bool try_lock_shared() {
    return mMutex.try_lock_shared();
  }

  void unlock_shared() {
    mMutex.unlock_shared();
  }

 private:
  struct MutableData {
    bool mIsApiLayerEnabled {false};
    bool mHaveOpenXR {false};
    bool mHaveHandTracking {false};
    bool mHaveHandTrackingAimFB {false};

    UltraleapStatus mUltraleapStatus {UltraleapStatus::NotFound};
    std::wstring mUltraleapPath;
  };

  const std::wstring mAPILayerPath;

  std::shared_mutex mMutex;
  MutableData mData;

  wil::unique_registry_watcher mRegWatcher;
  std::vector<std::function<void()>> mOnReload;

  void Load();
  void LoadRuntime();
  void LoadUltraleap();
};