// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include "FrameInfo.h"
#include "InputState.h"

namespace HandTrackedCockpitClicking {

class InputSource {
 public:
  virtual std::tuple<InputState, InputState>
  Update(PointerMode pointerMode, const FrameInfo& info) = 0;
};
}// namespace HandTrackedCockpitClicking
