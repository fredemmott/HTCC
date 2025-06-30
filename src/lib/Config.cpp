// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#include "Config.h"

#include <filesystem>

#include "DebugPrint.h"

namespace HandTrackedCockpitClicking::Config {

#define IT(native_type, name, defaultValue) native_type name {defaultValue};
HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, defaultValue) float name {defaultValue};
  HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
#define IT(name, defaultValue) std::string name {defaultValue};
    HandTrackedCockpitClicking_STRING_SETTINGS
#undef IT

  static const std::wstring BaseSubKey {
    L"SOFTWARE\\Fred Emmott\\HandTrackedCockpitClicking"};

static std::wstring GetCurrentExecutableFileName() {
  static std::wstring sCache {};
  if (!sCache.empty()) {
    return sCache;
  }
  wchar_t buf[MAX_PATH];
  const auto bufLen = GetModuleFileNameW(NULL, buf, MAX_PATH);

  sCache = std::filesystem::path(std::wstring_view {buf, bufLen})
             .filename()
             .wstring();
  return sCache;
}

static std::wstring AppOverrideSubKey(std::wstring_view executableFileName) {
  return std::format(L"{}\\AppOverrides\\{}", BaseSubKey, executableFileName);
}

template <class T>
static void
LoadDWord(const std::wstring& subKey, T& value, const wchar_t* valueName) {
  DWORD data {};
  DWORD dataSize = sizeof(data);
  const auto result = RegGetValueW(
    HKEY_LOCAL_MACHINE,
    subKey.c_str(),
    valueName,
    RRF_RT_DWORD,
    nullptr,
    &data,
    &dataSize);
  if (result == ERROR_SUCCESS) {
    value = static_cast<T>(data);
  }
}

static bool LoadString(
  const std::wstring& subKey,
  std::string& value,
  const wchar_t* valueName) {
  DWORD dataSize = 0;
  const auto sizeResult = RegGetValueW(
    HKEY_LOCAL_MACHINE,
    subKey.c_str(),
    valueName,
    RRF_RT_REG_SZ,
    nullptr,
    nullptr,
    &dataSize);
  if (sizeResult != ERROR_SUCCESS && sizeResult != ERROR_MORE_DATA) {
    return false;
  }

  std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t), L'\0');
  const auto dataResult = RegGetValueW(
    HKEY_LOCAL_MACHINE,
    subKey.c_str(),
    valueName,
    RRF_RT_REG_SZ,
    nullptr,
    buffer.data(),
    &dataSize);

  if (dataResult == ERROR_SUCCESS) {
    value = Utf8::FromWide(buffer.data());
    return true;
  }
  return false;
}

static void
LoadFloat(const std::wstring& subKey, float& value, const wchar_t* valueName) {
  std::string buffer;
  if (LoadString(subKey, buffer, valueName)) {
    value = std::atof(buffer.data());
  }
}

static void Load(const std::wstring& subKey) {
#define IT(native_type, name, defaultValue) \
  LoadDWord(subKey, Config::name, L#name);
  HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, defaultValue) LoadFloat(subKey, Config::name, L#name);
    HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
#define IT(name, defaultValue) LoadString(subKey, Config::name, L#name);
    HandTrackedCockpitClicking_STRING_SETTINGS
#undef IT
}

void LoadBaseConfig() {
  DebugPrint(L"Loading settings from HKLM\\{}", BaseSubKey);
  Load(BaseSubKey);
}

void LoadForCurrentProcess() {
  LoadForExecutableFileName(GetCurrentExecutableFileName());
}

void LoadForExecutableFileName(std::wstring_view executableFileName) {
  LoadBaseConfig();

  const auto subKey = AppOverrideSubKey(executableFileName);
  DebugPrint(L"Loading app overrides from HKLM\\{}", subKey);
  Load(subKey);
}

template <class T>
static void SaveDWord(const wchar_t* valueName, T value) {
  auto data = static_cast<DWORD>(value);
  const auto result = RegSetKeyValueW(
    HKEY_LOCAL_MACHINE,
    BaseSubKey.c_str(),
    valueName,
    REG_DWORD,
    &data,
    sizeof(data));
  if (result != ERROR_SUCCESS) {
    auto message = std::format("Saving to registry failed: error {}", result);
    throw std::runtime_error(message);
  }
}

static void SaveString(const wchar_t* valueName, const std::string_view value) {
  const auto buffer = Utf8::ToWide(value);
  const auto result = RegSetKeyValueW(
    HKEY_LOCAL_MACHINE,
    BaseSubKey.c_str(),
    valueName,
    REG_SZ,
    buffer.data(),
    buffer.size() * sizeof(buffer[0]));
  if (result != ERROR_SUCCESS) {
    auto message = std::format("Saving to registry failed: error {}", result);
    throw std::runtime_error(message);
  }
}

static void SaveFloat(const wchar_t* valueName, float value) {
  const auto data = std::format("{}", value);
  SaveString(valueName, data);
}

#define IT(native_type, name, defaultValue) \
  void Save##name(native_type value) { \
    Config::name = value; \
    SaveDWord(L#name, Config::name); \
  }
HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, defaultValue) \
  void Save##name(float value) { \
    Config::name = value; \
    SaveFloat(L#name, Config::name); \
  }
  HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
#define IT(name, defaultValue) \
  void Save##name(float value) { \
    Config::name = value; \
    SaveString(L#name, Config::name); \
  }
  HandTrackedCockpitClicking_STRING_SETTINGS
#undef IT

}// namespace HandTrackedCockpitClicking::Config
