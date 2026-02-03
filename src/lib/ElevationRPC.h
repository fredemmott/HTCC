// Copyright 2026 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <wil/resource.h>

#include <filesystem>

namespace HandTrackedCockpitClicking::ElevationRPC {

int main(std::string_view pipeName);

class Client {
 public:
  Client();
  ~Client();

  void WriteHKLMDWord(std::string_view valueName, uint32_t value);
  void WriteHKLMString(std::string_view valueName, std::string_view value);
  void SetAPILayerEnabled(bool isEnabled);
  void MoveAPILayerToLast(const std::filesystem::path&);

  static Client& Get();

 private:
  wil::unique_hfile mPipe;
};

}// namespace HandTrackedCockpitClicking::ElevationRPC