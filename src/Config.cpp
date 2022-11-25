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

namespace DCSQuestHandTracking::Config {

bool Enabled = true;
bool CheckDCS = true;
uint8_t VerboseDebug = 0;
uint8_t MirrorEye = 1;

DCSQuestHandTracking::PointerSource PointerSource
  = DCSQuestHandTracking::PointerSource::OculusHandTracking;

bool PinchToClick = true;
bool PinchToScroll = true;

bool PointCtrlFCUClicks = true;

uint16_t PointCtrlCenterX = 32767;
uint16_t PointCtrlCenterY = 32767;
float PointCtrlRadiansPerUnitX = 2.65e-5f;
float PointCtrlRadiansPerUnitY = 2.65e-5f;

static constexpr wchar_t SubKey[] {L"SOFTWARE\\FredEmmott\\DCSHandTracking"};

template <class T>
void LoadDWord(T& value, const wchar_t* valueName) {
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
void LoadFloat(float& value, const wchar_t* valueName) {
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
  RegGetValueW(
    HKEY_LOCAL_MACHINE,
    SubKey,
    valueName,
    RRF_RT_REG_SZ,
    nullptr,
    buffer.data(),
    &dataSize);
  value = static_cast<float>(_wtof(buffer.data()));
}

#define LOAD_DWORD(x) LoadDWord(Config::x, L#x);
#define LOAD_FLOAT(x) LoadFloat(Config::x, L#x);

void Load() {
  DebugPrint(L"Loading settings from HKLM\\{}", SubKey);
  LOAD_DWORD(Enabled);
  LOAD_DWORD(CheckDCS);
  LOAD_DWORD(VerboseDebug);
  LOAD_DWORD(MirrorEye);
  LOAD_DWORD(PinchToClick);
  LOAD_DWORD(PinchToScroll);

  LOAD_DWORD(PointCtrlFCUClicks);

  LOAD_DWORD(PointCtrlCenterX);
  LOAD_DWORD(PointCtrlCenterY);
  LOAD_DWORD(PointCtrlRadiansPerUnitX);
  LOAD_DWORD(PointCtrlRadiansPerUnitY);
}

}// namespace DCSQuestHandTracking::Config
