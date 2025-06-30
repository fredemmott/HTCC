// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once
#include <optional>

#include "PointerMode.h"
#include "openxr.h"

namespace HandTrackedCockpitClicking {

struct ActionState {
  bool mPrimary {false};// 'left click'
  bool mSecondary {false};// 'right click'

  enum class ValueChange {
    None,
    Decrease,// scroll wheel up
    Increase,// scroll wheel down
  };
  ValueChange mValueChange {ValueChange::None};

  constexpr bool Any() const {
    return mPrimary || mSecondary || (mValueChange != ValueChange::None);
  }

  constexpr auto operator<=>(const ActionState&) const noexcept = default;
};

struct InputState {
  XrHandEXT mHand;
  XrTime mPositionUpdatedAt {};

  PointerMode mPointerMode {PointerMode::None};
  // in LOCAL space
  std::optional<XrPosef> mPose;
  /* Rotation around the x and y axis, in radians.
   *
   * Movement *along* the x axis is rotation *around* the y axis.
   */
  std::optional<XrVector2f> mDirection;

  ActionState mActions {};
};

}// namespace HandTrackedCockpitClicking
