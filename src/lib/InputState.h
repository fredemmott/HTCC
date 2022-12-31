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
