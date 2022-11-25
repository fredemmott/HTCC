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

#include <openxr/openxr.h>

#include <unordered_map>
#include <unordered_set>

#include "ActionState.h"
#include "Config.h"
#include "OpenXRNext.h"

namespace DCSQuestHandTracking {

class VirtualControllerSink final {
 public:
  VirtualControllerSink(const std::shared_ptr<OpenXRNext>& oxr);

  void Update(
    const std::optional<XrPosef>& leftAimPose,
    const std::optional<XrPosef>& rightAimPose,
    const ActionState& actionState);

  XrResult xrSuggestInteractionProfileBindings(
    XrInstance instance,
    const XrInteractionProfileSuggestedBinding* suggestedBindings);

  XrResult xrCreateActionSpace(
    XrSession session,
    const XrActionSpaceCreateInfo* createInfo,
    XrSpace* space);

  XrResult xrGetActionStateFloat(
    XrSession session,
    const XrActionStateGetInfo* getInfo,
    XrActionStateFloat* state);

 private:
  // based on /interaction_profiles/oculus/touch_controller
  struct ControllerState {
    bool present {true};

    XrPosef aimPose {};
    XrSpace aimSpace {};
    XrAction aimAction {};

    XrPosef gripPose {};
    XrSpace gripSpace {};
    XrAction gripAction {};

    float thumbstickX {0.0f};
    float thumbstickY {0.0f};
    bool thumbstickClick {false};
    bool thumbstickTouch {true};
    float triggerValue {0.0f};
    bool triggerTouch {false};
    std::unordered_set<XrAction> squeezeValue {};
  };
  struct LeftControllerState final : public ControllerState {
    bool menuClick {false};
    bool xClick {false};
    bool xTouch {false};
    bool yClick {false};
    bool yTouch {false};
  };
  struct RightControllerState final : public ControllerState {
    bool aClick {false};
    bool aTouch {false};
    bool bClick {false};
    bool bTouch {false};
  };

  std::shared_ptr<OpenXRNext> mOpenXR;

  LeftControllerState mLeftHand {};
  RightControllerState mRightHand {};

  std::unordered_map<XrAction, std::string> mActionPaths;
};

}// namespace DCSQuestHandTracking
