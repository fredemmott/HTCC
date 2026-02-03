// Copyright 2026 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "ElevationRPC.h"

#include <SoftPub.h>
#include <shellapi.h>
#include <wil/registry.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wintrust.h>

#include <cinttypes>
#include <filesystem>
#include <glaze/glaze.hpp>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <variant>

#include "Utf8.h"

namespace {
template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

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

[[nodiscard]]
bool IsTrustedProcess(HANDLE const handle) {
  const auto path = wil::QueryFullProcessImageNameW(handle);
  return IsTrustedExecutable(path.get());
}

}// namespace

namespace HandTrackedCockpitClicking::ElevationRPC {
static constexpr wchar_t BaseSubKey[] {
  L"SOFTWARE\\Fred Emmott\\HandTrackedCockpitClicking"};
static constexpr wchar_t APILayerSubkey[]
  = L"SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\Implicit";

enum class MessageType {
  write_hklm_dword,
  write_hklm_string,
  set_api_layer_enabled,
  move_api_layer_to_last,
  bye,
};

struct write_hklm_dword_t {
  static constexpr auto type = MessageType::write_hklm_dword;
  std::string_view value_name;
  uint32_t value;

  void operator()() const {
    wil::reg::set_value_dword(
      HKEY_LOCAL_MACHINE, BaseSubKey, Utf8::ToWide(value_name).c_str(), value);
  }
};

struct write_hklm_string_t {
  static constexpr auto type = MessageType::write_hklm_string;
  std::string_view value_name;
  std::string_view value;
  void operator()() const {
    wil::reg::set_value_string(
      HKEY_LOCAL_MACHINE,
      BaseSubKey,
      Utf8::ToWide(value_name).c_str(),
      Utf8::ToWide(value).c_str());
  }
};

struct set_api_layer_enabled_t {
  static constexpr auto type = MessageType::set_api_layer_enabled;
  bool is_enabled {};
  void operator()() const {
    const auto thisExecutable = wil::GetModuleFileNameW(nullptr);
    const auto path = std::filesystem::path {thisExecutable.get()}.parent_path()
      / "APILayer.json";

    const DWORD disabled = is_enabled ? 0 : 1;
    wil::reg::set_value_dword(
      HKEY_LOCAL_MACHINE, APILayerSubkey, path.c_str(), disabled);
  }
};

struct move_api_layer_to_last_t {
  static constexpr auto type = MessageType::move_api_layer_to_last;
  std::string_view layer;

  void operator()() const {
    const auto wideLayer = Utf8::ToWide(layer);
    const auto value = wil::reg::get_value_dword(
      HKEY_LOCAL_MACHINE, APILayerSubkey, wideLayer.c_str());
    RegDeleteKeyValueW(HKEY_LOCAL_MACHINE, APILayerSubkey, wideLayer.c_str());
    wil::reg::set_value_dword(
      HKEY_LOCAL_MACHINE, APILayerSubkey, wideLayer.c_str(), value);
  }
};

struct bye_t {
  static constexpr auto type = MessageType::bye;
};

using message_t = std::variant<
  write_hklm_dword_t,
  write_hklm_string_t,
  set_api_layer_enabled_t,
  move_api_layer_to_last_t,
  bye_t>;
}// namespace HandTrackedCockpitClicking::ElevationRPC

template <>
struct glz::meta<HandTrackedCockpitClicking::ElevationRPC::message_t> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = []<class... Ts>(
                                std::type_identity<std::variant<Ts...>>) {
    return std::array {std::to_underlying(Ts::type)...};
  }(std::type_identity<HandTrackedCockpitClicking::ElevationRPC::message_t> {});
};

namespace HandTrackedCockpitClicking::ElevationRPC {
int main(const std::string_view pipeName) {
  const wil::unique_hfile pipe {CreateFileW(
    Utf8::ToWide(pipeName).c_str(),
    GENERIC_READ | GENERIC_WRITE,
    0,
    nullptr,
    OPEN_EXISTING,
    FILE_FLAG_OVERLAPPED,
    nullptr)};

  if (!pipe) {
    return EXIT_FAILURE;
  }
  const wil::unique_event readEvent {wil::EventOptions::None};
  OVERLAPPED overlapped {.hEvent = readEvent.get()};

  ULONG parentPid {};
  if (!GetNamedPipeServerProcessId(pipe.get(), &parentPid)) {
    return EXIT_FAILURE;
  }
  const wil::unique_process_handle parent {OpenProcess(
    SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentPid)};
  if (!parent) {
    return EXIT_FAILURE;
  }

  if (!IsTrustedProcess(parent.get())) {
    return EXIT_FAILURE;
  }

  while (true) {
    uint32_t byteCount {};
    const bool immediateRead = ReadFile(
      pipe.get(), &byteCount, sizeof(byteCount), nullptr, &overlapped);

    if (!immediateRead) {
      if (GetLastError() != ERROR_IO_PENDING) {
        return EXIT_FAILURE;
      }

      const std::array handles {readEvent.get(), parent.get()};
      const auto result = WaitForMultipleObjects(
        handles.size(), handles.data(), FALSE, INFINITE);
      if (result == (WAIT_OBJECT_0 + 1)) {
        // parent process gone
        return EXIT_SUCCESS;
      }
    }

    DWORD bytesRead {};
    if (!GetOverlappedResult(pipe.get(), &overlapped, &bytesRead, FALSE)) {
      return EXIT_FAILURE;
    }
    if (bytesRead != sizeof(byteCount)) {
      return EXIT_FAILURE;
    }
    if (byteCount == 0) {
      return EXIT_FAILURE;
    }

    std::string json(byteCount, '\0');
    ReadFile(pipe.get(), json.data(), byteCount, nullptr, &overlapped);
    if (!GetOverlappedResult(pipe.get(), &overlapped, &bytesRead, TRUE)) {
      return EXIT_FAILURE;
    }
    if (bytesRead != byteCount) {
      return EXIT_FAILURE;
    }

    const auto message = glz::read_json<message_t>(json);
    if (!message) {
      return EXIT_FAILURE;
    }
    std::optional<int> exitCode = std::visit(
      overload {
        [&](const bye_t&) -> std::optional<int> { return EXIT_SUCCESS; },
        [&](const std::invocable<> auto& it) -> std::optional<int> {
          it();
          return std::nullopt;
        },
      },
      *message);
    if (exitCode) {
      return *exitCode;
    }
  }
}

void send(HANDLE const pipe, const auto& payload) {
  const message_t message {payload};
  const auto json = glz::write_json(message);
  DWORD bytesWritten {};
  uint32_t jsonSize {static_cast<uint32_t>(json->size())};
  WriteFile(pipe, &jsonSize, sizeof(jsonSize), &bytesWritten, nullptr);
  WriteFile(pipe, json->data(), json->size(), &bytesWritten, nullptr);
  FlushFileBuffers(pipe);
}

Client::Client() {
  UUID uuid {};
  UuidCreate(&uuid);
  wil::unique_rpc_wstr guidStr;
  UuidToStringW(&uuid, guidStr.put());

  const auto pipeName = std::format(
    LR"(\\.\pipe\HTCC_ElevationHelper_{})",
    std::wstring_view {reinterpret_cast<wchar_t*>(guidStr.get())});

  mPipe.reset(CreateNamedPipeW(
    pipeName.c_str(),
    PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
      | PIPE_REJECT_REMOTE_CLIENTS,
    1,
    1024,
    1024,
    NMPWAIT_USE_DEFAULT_WAIT,
    nullptr));

  const auto thisExe = wil::GetModuleFileNameW(nullptr);
  const auto parameters = std::format(L"--elevated-helper-pipe {}", pipeName);

  SHELLEXECUTEINFOW sei {
    .cbSize = sizeof(sei),
    .lpVerb = L"runas",
    .lpFile = thisExe.get(),
    .lpParameters = parameters.c_str(),
  };
  if (!ShellExecuteExW(&sei)) {
    __debugbreak();
    return;
  }

  ConnectNamedPipe(mPipe.get(), nullptr);
}

Client::~Client() {
  send(mPipe.get(), bye_t {});
}

void Client::WriteHKLMDWord(
  const std::string_view valueName,
  const uint32_t value) {
  send(mPipe.get(), write_hklm_dword_t {valueName, value});
}
void Client::WriteHKLMString(
  const std::string_view valueName,
  const std::string_view value) {
  send(mPipe.get(), write_hklm_string_t {valueName, value});
}
void Client::SetAPILayerEnabled(const bool isEnabled) {
  send(mPipe.get(), set_api_layer_enabled_t {isEnabled});
}

void Client::MoveAPILayerToLast(const std::filesystem::path& path) {
  send(mPipe.get(), move_api_layer_to_last_t {path.string()});
}

Client& Client::Get() {
  static Client instance;
  return instance;
}

}// namespace HandTrackedCockpitClicking::ElevationRPC