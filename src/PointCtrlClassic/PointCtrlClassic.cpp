// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT

// clang-format off
#define _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Unknwn.h>

#include <Psapi.h>
#include <TlHelp32.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <thread>
#include <vector>

#include "Config.h"
#include "PointCtrlSource.h"
#include "VirtualTouchScreenSink.h"

using namespace HandTrackedCockpitClicking;
namespace Config = HandTrackedCockpitClicking::Config;

static DWORD FindDCSProcessID() {
  const wil::unique_tool_help_snapshot snapshot {
    CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL)};

  PROCESSENTRY32 pe {sizeof(PROCESSENTRY32)};
  if (!Process32First(snapshot.get(), &pe)) {
    return 0;
  }

  do {
    const auto pid = pe.th32ProcessID;
    wil::unique_process_handle process {
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)};
    if (!process) {
      continue;
    }

    wchar_t pathBuf[MAX_PATH];
    const auto pathLen
      = GetProcessImageFileNameW(process.get(), pathBuf, std::size(pathBuf));
    if (pathLen == 0) {
      continue;
    }
    const auto filename
      = std::filesystem::path(std::wstring_view {pathBuf, pathLen}).filename();
    if (filename == L"DCS.exe") {
      return pid;
    }
  } while (Process32Next(snapshot.get(), &pe));
  return 0;
}

int main() {
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  Config::LoadForExecutableFileName(L"DCS.exe");
  Config::PointerSource = PointerSource::PointCtrl;
  Config::PointerSink = PointerSink::VirtualTouchScreen;
  Config::ClickActionSink = ActionSink::VirtualTouchScreen;
  Config::ScrollActionSink = ActionSink::VirtualTouchScreen;

  const auto headsetCalibration
    = VirtualTouchScreenSink::CalibrationFromConfig();
  if (!headsetCalibration) {
    std::cerr << "Run the HTCC PointCTRL calibration utility first.\n"
              << "Press any key to exit." << std::endl;
    std::cin.get();
    return 1;
  }

  wil::unique_event pointCtrlEvent {CreateEventW(nullptr, FALSE, FALSE, nullptr)};
  PointCtrlSource pointCtrl(pointCtrlEvent.get());
  if (!pointCtrl.IsConnected()) {
    std::cout << "Connect your PointCTRL, or press Ctrl+C to exit" << std::endl;
    do {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (!pointCtrl.IsConnected());
  }
  std::cout << "Found your PointCTRL." << std::endl;
  while (true) {
    std::cout << "Looking for DCS..." << std::endl;
    DWORD dcsPID {};
    while (!(dcsPID = FindDCSProcessID())) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << std::format("...found DCS (PID {})", dcsPID) << std::endl;
    std::cout << "Setting up virtual touch screen..." << std::endl;

    VirtualTouchScreenSink touchScreen(*headsetCalibration, dcsPID);

    std::cout << "Running - press Ctrl+C to exit." << std::endl;
    while (true) {
      WaitForSingleObject(pointCtrlEvent.get(), INFINITE);
      FrameInfo frameInfo;
      frameInfo.mNow = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count();
      auto [left, right] = pointCtrl.Update(PointerMode::Direction, frameInfo);
      touchScreen.Update(left, right);
    }
  }
  return 0;
}
