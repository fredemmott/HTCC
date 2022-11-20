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

#include "APILayer.h"

#include <loader_interfaces.h>
#include <openxr/openxr.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "DebugPrint.h"
#include "OpenXRNext.h"

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace DCSQuestHandTracking {

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

constexpr std::string_view OpenXRLayerName {
  "XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking"};
static_assert(OpenXRLayerName.size() <= XR_MAX_API_LAYER_NAME_SIZE);

static std::shared_ptr<OpenXRNext> gNext;
static APILayer* gInstance = nullptr;
static bool gIsDCS = true;

APILayer::APILayer(XrSession session, const std::shared_ptr<OpenXRNext>& next)
  : mOpenXR(next) {
  DebugPrint("{}()", __FUNCTION__);
  auto oxr = next.get();

  XrReferenceSpaceCreateInfo referenceSpace {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .next = nullptr,
    .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
    .poseInReferenceSpace = XR_POSEF_IDENTITY,
  };

  XrResult nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mLocalSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create local space: {}", nextResult);
    return;
  }

  referenceSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mViewSpace);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create view space: {}", nextResult);
    return;
  }

  InitHandTrackers(session);
  DebugPrint("Fully initialized.");
}

void APILayer::InitHandTrackers(XrSession session) {
  if (mLeftHand && mRightHand) [[likely]] {
    return;
  }
  XrHandTrackerCreateInfoEXT createInfo {
    .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
    .hand = XR_HAND_LEFT_EXT,
    .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
  };
  auto nextResult
    = mOpenXR->xrCreateHandTrackerEXT(session, &createInfo, &mLeftHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize left hand: {}", nextResult);
    return;
  }
  createInfo.hand = XR_HAND_RIGHT_EXT;
  nextResult
    = mOpenXR->xrCreateHandTrackerEXT(session, &createInfo, &mRightHand);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to initialize right hand: {}", nextResult);
    return;
  }

  DebugPrint("Initialized hand trackers.");
}

APILayer::~APILayer() {
  if (mLocalSpace) {
    mOpenXR->xrDestroySpace(mLocalSpace);
  }
  if (mViewSpace) {
    mOpenXR->xrDestroySpace(mViewSpace);
  }
  if (mLeftHand) {
    mOpenXR->xrDestroyHandTrackerEXT(mLeftHand);
  }
  if (mRightHand) {
    mOpenXR->xrDestroyHandTrackerEXT(mRightHand);
  }
}

template <class Actual, class Wanted>
static constexpr bool HasFlags(Actual actual, Wanted wanted) {
  return (actual & wanted) == wanted;
}

static void DumpHandState(
  std::string_view name,
  const XrHandTrackingAimStateFB& state) {
  if (!HasFlags(state.status, XR_HAND_TRACKING_AIM_VALID_BIT_FB)) {
    DebugPrint("{} hand not present.", name);
    return;
  }

  DebugPrint(
    "{} hand: I{} M{} R{} L{} @ ({}, {}, {})",
    name,
    HasFlags(state.status, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB),
    HasFlags(state.status, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB),
    state.aimPose.position.x,
    state.aimPose.position.y,
    state.aimPose.position.z);
  DebugPrint(
    "  I{:.02f} M{:.02f} R{:.02f} L{:.02f}",
    state.pinchStrengthIndex,
    state.pinchStrengthMiddle,
    state.pinchStrengthRing,
    state.pinchStrengthLittle);
}

XrResult APILayer::xrEndFrame(
  XrSession session,
  const XrFrameEndInfo* frameEndInfo) {
  InitHandTrackers(session);

  XrHandJointsLocateInfoEXT locateInfo {
    .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
    .baseSpace = mViewSpace,
    .time = frameEndInfo->displayTime,
  };

  XrHandTrackingAimStateFB aimState {XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
  XrHandJointLocationEXT jointData[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT joints {
    .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
    .next = &aimState,
    .jointCount = XR_HAND_JOINT_COUNT_EXT,
    .jointLocations = jointData,
  };

  auto nextResult
    = mOpenXR->xrLocateHandJointsEXT(mLeftHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to get left hand position: {}", nextResult);
  }
  const auto left = aimState;
  nextResult = mOpenXR->xrLocateHandJointsEXT(mRightHand, &locateInfo, &joints);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to get right hand position: {}", nextResult);
  }
  const auto right = aimState;

  static std::chrono::steady_clock::time_point lastPrint {};
  const auto now = std::chrono::steady_clock::now();
  if (now - lastPrint > std::chrono::seconds(1)) {
    lastPrint = now;
    DumpHandState("Left", left);
    DumpHandState("Right", right);
  }

  if (!mVirtualTouchScreen) [[unlikely]] {
    mVirtualTouchScreen = std::make_unique<VirtualTouchScreen>(
      gNext, session, frameEndInfo->displayTime, mViewSpace);
  }
  mVirtualTouchScreen->SubmitData(left, right);

  return mOpenXR->xrEndFrame(session, frameEndInfo);
}

static XrResult xrEndFrame(
  XrSession session,
  const XrFrameEndInfo* frameEndInfo) {
  if (gInstance) {
    return gInstance->xrEndFrame(session, frameEndInfo);
  }
  return gNext->xrEndFrame(session, frameEndInfo);
}

static XrResult xrCreateSession(
  XrInstance instance,
  const XrSessionCreateInfo* createInfo,
  XrSession* session) {
  auto nextResult = gNext->xrCreateSession(instance, createInfo, session);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create OpenXR session: {}", nextResult);
    return nextResult;
  }

  if (!gIsDCS) {
    return XR_SUCCESS;
  }

  DebugPrint("Hand tracking available, starting up!");
  gInstance = new APILayer(*session, gNext);

  return XR_SUCCESS;
}

static XrResult xrDestroySession(XrSession session) {
  delete gInstance;
  gInstance = nullptr;
  return gNext->xrDestroySession(session);
}

static XrResult xrDestroyInstance(XrInstance instance) {
  delete gInstance;
  gInstance = nullptr;
  return gNext->xrDestroyInstance(instance);
}

static XrResult xrGetInstanceProcAddr(
  XrInstance instance,
  const char* name_cstr,
  PFN_xrVoidFunction* function) {
  std::string_view name {name_cstr};

  if (name == "xrCreateSession") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrCreateSession);
    return XR_SUCCESS;
  }
  if (name == "xrDestroySession") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrDestroySession);
    return XR_SUCCESS;
  }
  if (name == "xrDestroyInstance") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrDestroyInstance);
    return XR_SUCCESS;
  }
  if (name == "xrEndFrame") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrEndFrame);
    return XR_SUCCESS;
  }

  if (gNext) {
    return gNext->xrGetInstanceProcAddr(instance, name_cstr, function);
  }

  if (name == "xrEnumerateApiLayerProperties") {
    // No need to do anything here; should be implemented by the runtime
    return XR_SUCCESS;
  }

  DebugPrint(
    "Unsupported OpenXR call '{}' with instance {:#016x} and no next",
    name,
    reinterpret_cast<uintptr_t>(instance));
  return XR_ERROR_FUNCTION_UNSUPPORTED;
}

static bool HaveHandTracking(OpenXRNext* oxr) {
  uint32_t extensionCount = 0;

  auto nextResult = oxr->xrEnumerateInstanceExtensionProperties(
    nullptr, 0, &extensionCount, nullptr);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Getting extension count failed: {}", nextResult);
    return false;
  }

  if (extensionCount == 0) {
    DebugPrint(
      "Runtime supports no extensions, so definitely doesn't support hand "
      "tracking. Reporting success but doing nothing.");
    return false;
  }

  std::vector<XrExtensionProperties> extensions(
    extensionCount, XrExtensionProperties {XR_TYPE_EXTENSION_PROPERTIES});
  nextResult = oxr->xrEnumerateInstanceExtensionProperties(
    nullptr, extensionCount, &extensionCount, extensions.data());
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Enumerating extensions failed: {}", nextResult);
    return false;
  }

  bool foundHandTracking = false;
  bool foundHandTrackingAim = false;
  for (const auto& it: extensions) {
    const std::string_view name {it.extensionName};
    DebugPrint("Found {}", name);

    if (name == XR_EXT_HAND_TRACKING_EXTENSION_NAME) {
      foundHandTracking = true;
      continue;
    }
    if (name == XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME) {
      foundHandTrackingAim = true;
      continue;
    }
  }

  if (!foundHandTracking) {
    DebugPrint(
      "Did not find {}, doing nothing.", XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    return false;
  }

  if (!foundHandTrackingAim) {
    DebugPrint(
      "Did not find {}, doing nothing.",
      XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
    return false;
  }

  DebugPrint("Hand tracking extensions are available");
  return true;
}

static XrResult xrCreateApiLayerInstance(
  const XrInstanceCreateInfo* originalInfo,
  const struct XrApiLayerCreateInfo* layerInfo,
  XrInstance* instance) {
  DebugPrint("{}", __FUNCTION__);

  //  TODO: check version fields etc in layerInfo

  XrInstanceCreateInfo info {XR_TYPE_INSTANCE_CREATE_INFO};
  if (originalInfo) {
    info = *originalInfo;
  }
  // no instance
  OpenXRNext next(NULL, layerInfo->nextInfo->nextGetInstanceProcAddr);
  std::vector<const char*> enabledExtensions;
  if (gIsDCS && HaveHandTracking(&next)) {
    for (auto i = 0; i < originalInfo->enabledExtensionCount; ++i) {
      enabledExtensions.push_back(originalInfo->enabledExtensionNames[i]);
    }
    if (
      std::ranges::find(
        enabledExtensions,
        std::string_view {XR_EXT_HAND_TRACKING_EXTENSION_NAME})
      == enabledExtensions.end()) {
      enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    }
    if (
      std::ranges::find(
        enabledExtensions,
        std::string_view {XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME})
      == enabledExtensions.end()) {
      enabledExtensions.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
    }
    info.enabledExtensionCount = enabledExtensions.size();
    info.enabledExtensionNames = enabledExtensions.data();
  }

  XrApiLayerCreateInfo nextLayerInfo = *layerInfo;
  nextLayerInfo.nextInfo = layerInfo->nextInfo->next;

  auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
    &info, &nextLayerInfo, instance);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Next failed.");
    return nextResult;
  }

  gNext = std::make_shared<OpenXRNext>(
    *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);

  return XR_SUCCESS;
}

}// namespace DCSQuestHandTracking

using namespace DCSQuestHandTracking;

extern "C" {
XrResult __declspec(dllexport) XRAPI_CALL
  DCSQuestHandTracking_xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo* loaderInfo,
    const char* layerName,
    XrNegotiateApiLayerRequest* apiLayerRequest) {
  if (layerName != DCSQuestHandTracking::OpenXRLayerName) {
    DebugPrint("Layer name mismatch:\n -{}\n +{}", OpenXRLayerName, layerName);
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  wchar_t executablePath[MAX_PATH];
  const auto executablePathLen
    = GetModuleFileNameW(NULL, executablePath, MAX_PATH);
  const auto exeName = std::filesystem::path(
                         std::wstring_view {executablePath, executablePathLen})
                         .filename();
  if (exeName != L"DCS.exe") {
    DebugPrint(L"Doing nothing - '{}' is not 'DCS.exe'", exeName.wstring());
    gIsDCS = false;
  }

  // TODO: check version fields etc in loaderInfo

  apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
  apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
  apiLayerRequest->getInstanceProcAddr
    = &DCSQuestHandTracking::xrGetInstanceProcAddr;
  apiLayerRequest->createApiLayerInstance
    = &DCSQuestHandTracking::xrCreateApiLayerInstance;
  return XR_SUCCESS;
}
}
