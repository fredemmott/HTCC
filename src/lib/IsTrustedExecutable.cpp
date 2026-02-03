// Copyright 2026 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "IsTrustedExecutable.h"

#include <SoftPub.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wintrust.h>

namespace HandTrackedCockpitClicking {

[[nodiscard]]
bool IsTrustedExecutable(const std::filesystem::path& executable) {
  const auto myPathString = wil::GetModuleFileNameW(nullptr);
  const auto parent
    = canonical(std::filesystem::path {myPathString.get()}.parent_path());
  if (canonical(executable.parent_path()) != parent) {
    return false;
  }

  const auto fileName = executable.filename().string();
  if (
    fileName != "HTCCSettings.exe" && fileName != "PointCtrlCalibration.exe") {
    return false;
  }

  WINTRUST_FILE_INFO fileInfo {
    .cbStruct = sizeof(fileInfo),
    .pcwszFilePath = executable.c_str(),
  };
  WINTRUST_DATA trustData {
    .cbStruct = sizeof(trustData),
    .dwUIChoice = WTD_UI_NONE,
    .fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN,
    .dwUnionChoice = WTD_CHOICE_FILE,
    .pFile = &fileInfo,
    .dwStateAction = WTD_STATEACTION_VERIFY,
    .dwProvFlags = WTD_REVOCATION_CHECK_CHAIN,
    .dwUIContext = WTD_UICONTEXT_EXECUTE,
  };
  GUID actionGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
  const auto trusted = WinVerifyTrust(nullptr, &actionGuid, &trustData) == 0;
  trustData.dwStateAction = WTD_STATEACTION_CLOSE;
  WinVerifyTrust(nullptr, &actionGuid, &trustData);
  if (trusted) {
    return true;
  }
#ifdef ALLOW_UNSIGNED_EXECUTABLES
  if (!trusted) {
    OutputDebugStringW(
      std::format(
        L"Allowing unsigned executable `{}` due to build config\n",
        executable.wstring())
        .c_str());
  }
  return true;
#else
  MessageBoxW(
    nullptr,
    std::format(
      L"{} has been tampered with; you should scan your computer for malware.",
      executable.filename().wstring())
      .c_str(),
    L"HTCC",
    MB_OK | MB_ICONERROR);
  return false;
#endif
}

}// namespace HandTrackedCockpitClicking
