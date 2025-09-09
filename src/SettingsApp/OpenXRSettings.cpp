// Copyright (c) 2024-present Frederick Emmott
// SPDX-License-Identifier: MIT
#include "OpenXRSettings.h"

#include <openxr/openxr.h>

#include <filesystem>

namespace {
std::wstring GetAPILayerPath() {
  wchar_t buf[MAX_PATH];
  const auto bufLen = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  return std::filesystem::weakly_canonical(
           std::filesystem::path(std::wstring_view {buf, bufLen}).parent_path()
           / "APILayer.json")
    .wstring();
}

bool IsAPILayerEnabled() noexcept {
  const auto path = GetAPILayerPath();
  const auto disabled
    = wil::reg::try_get_value_dword(
        HKEY_LOCAL_MACHINE, OpenXRSettings::APILayerSubkey, path.c_str())
        .value_or(1);
  return !disabled;
}
}// namespace

OpenXRSettings::OpenXRSettings() : mAPILayerPath(GetAPILayerPath()) {
  this->Load();

  mRegWatcher = wil::make_registry_watcher(
    HKEY_LOCAL_MACHINE, OpenXRSubkey, true, [this](auto) {
      this->Load();
      const auto lock = std::shared_lock(mMutex);
      for (auto&& onReload: mOnReload) {
        onReload();
      }
    });
}

void OpenXRSettings::Load() {
  const auto lock = std::unique_lock(mMutex);

  mData = {};
  mData.mIsApiLayerEnabled = IsAPILayerEnabled();

  this->LoadRuntime();
  this->LoadUltraleap();
}

void OpenXRSettings::LoadRuntime() {
  uint32_t extensionCount = 0;
  mData.mHaveOpenXR = XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(
    nullptr, 0, &extensionCount, nullptr));
  if (mData.mHaveOpenXR) {
    std::vector<XrExtensionProperties> extensions(
      extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(
          nullptr, extensions.size(), &extensionCount, extensions.data()))) {
      for (auto&& extension: extensions) {
        constexpr std::string_view handTrackingExt {
          XR_EXT_HAND_TRACKING_EXTENSION_NAME};
        constexpr std::string_view fbHandTrackingAim {
          XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME};
        if (extension.extensionName == handTrackingExt) {
          mData.mHaveHandTracking = true;
        }
        if (extension.extensionName == fbHandTrackingAim) {
          mData.mHaveHandTrackingAimFB = true;
        }
      }
    }
  }
}

void OpenXRSettings::LoadUltraleap() {
  wil::unique_hkey key;
  if (!SUCCEEDED(
        wil::reg::open_unique_key_nothrow(
          HKEY_LOCAL_MACHINE, APILayerSubkey, key))) {
    return;
  }

  bool haveHTCC = false;

  for (auto&& data: wil::make_range(
         wil::reg::value_iterator(key.get()), wil::reg::value_iterator())) {
    if (data.name == GetAPILayerPath()) {
      haveHTCC = true;
      continue;
    }
    if (!data.name.ends_with(L"\\UltraleapHandTracking.json")) {
      continue;
    }
    DWORD disabled {};
    if (!SUCCEEDED(
          wil::reg::get_value_dword_nothrow(
            key.get(), data.name.c_str(), &disabled))) {
      continue;
    }

    mData.mUltraleapPath = data.name;

    if (disabled) {
      mData.mUltraleapStatus = UltraleapStatus::DisabledInRegistry;
      continue;
    }

    if (
      std::getenv("DISABLE_XR_APILAYER_ULTRALEAP_HAND_TRACKING_1") != nullptr) {
      mData.mUltraleapStatus = UltraleapStatus::DisabledByEnvironmentVariable;
      return;
    }

    mData.mUltraleapStatus
      = haveHTCC ? UltraleapStatus::HTCCFirst : UltraleapStatus::UltraleapFirst;
    return;
  }
}

OpenXRSettings::~OpenXRSettings() = default;
