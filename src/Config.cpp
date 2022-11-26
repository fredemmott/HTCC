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
#include "Config.h"

#include "DebugPrint.h"

namespace HandTrackedCockpitClicking::Config {

#define IT(native_type, name, default) native_type name {default};
HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, default) float name {default};
  HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT

  static constexpr wchar_t SubKey[] {
    L"SOFTWARE\\FredEmmott\\HandTrackedCockpitClicking"};

template <class T>
static void LoadDWord(T& value, const wchar_t* valueName) {
  DWORD data {};
  DWORD dataSize = sizeof(data);
  const auto hklmResult = RegGetValueW(
    HKEY_LOCAL_MACHINE,
    SubKey,
    valueName,
    RRF_RT_DWORD,
    nullptr,
    &data,
    &dataSize);
  if (hklmResult == ERROR_SUCCESS) {
    value = static_cast<T>(data);
  }
}

static void LoadFloat(float& value, const wchar_t* valueName) {
  DWORD dataSize = 0;
  RegGetValueW(
    HKEY_LOCAL_MACHINE,
    SubKey,
    valueName,
    RRF_RT_REG_SZ,
    nullptr,
    nullptr,
    &dataSize);
  std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t), L'\0');
  if (
    RegGetValueW(
      HKEY_LOCAL_MACHINE,
      SubKey,
      valueName,
      RRF_RT_REG_SZ,
      nullptr,
      buffer.data(),
      &dataSize)
    != ERROR_SUCCESS) {
    return;
  }
  value = static_cast<float>(_wtof(buffer.data()));
}

void Load() {
  DebugPrint(L"Loading settings from HKLM\\{}", SubKey);

#define IT(native_type, name, default) LoadDWord(Config::name, L#name);
  HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, default) LoadFloat(Config::name, L#name);
    HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
}

template <class T>
static void SaveDWord(const wchar_t* valueName, T value) {
  auto data = static_cast<DWORD>(value);
  const auto result = RegSetKeyValueW(
    HKEY_LOCAL_MACHINE, SubKey, valueName, REG_DWORD, &data, sizeof(data));
  if (result != ERROR_SUCCESS) {
    auto message = std::format("Saving to registry failed: error {}", result);
    throw std::runtime_error(message);
  }
}

static void SaveFloat(const wchar_t* valueName, float value) {
  const auto data = std::format(L"{}", value);

  const auto result = RegSetKeyValueW(
    HKEY_LOCAL_MACHINE,
    SubKey,
    valueName,
    REG_SZ,
    data.data(),
    data.size() * sizeof(data[0]));
  if (result != ERROR_SUCCESS) {
    auto message = std::format("Saving to registry failed: error {}", result);
    throw std::runtime_error(message);
  }
}

void Save() {
#define IT(native_type, name, default) SaveDWord(L#name, Config::name);
  HandTrackedCockpitClicking_DWORD_SETTINGS
#undef IT
#define IT(name, default) SaveFloat(L#name, Config::name);
    HandTrackedCockpitClicking_FLOAT_SETTINGS
#undef IT
}

}// namespace HandTrackedCockpitClicking::Config
