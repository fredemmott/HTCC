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

#include <memory>
#include <string>

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

APILayer::APILayer(XrSession session, const std::shared_ptr<OpenXRNext>& next)
  : mOpenXR(next) {
  DebugPrint("{}", __FUNCTION__);
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
    return;
  }

  referenceSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  nextResult
    = oxr->xrCreateReferenceSpace(session, &referenceSpace, &mViewSpace);
  if (nextResult != XR_SUCCESS) {
    return;
  }
}

APILayer::~APILayer() {
  if (mLocalSpace) {
    mOpenXR->xrDestroySpace(mLocalSpace);
  }
  if (mViewSpace) {
    mOpenXR->xrDestroySpace(mViewSpace);
  }
}

static APILayer* gInstance = nullptr;

XrResult xrCreateSession(
  XrInstance instance,
  const XrSessionCreateInfo* createInfo,
  XrSession* session) {
  XrInstanceProperties instanceProps {XR_TYPE_INSTANCE_PROPERTIES};
  gNext->xrGetInstanceProperties(instance, &instanceProps);
  DebugPrint(
    "OpenXR runtime: '{}' v{:#x}",
    instanceProps.runtimeName,
    instanceProps.runtimeVersion);

  auto nextResult = gNext->xrCreateSession(instance, createInfo, session);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("next xrCreateSession failed: {}", nextResult);
    return nextResult;
  }

  uint32_t count = 0;
  nextResult = gNext->xrEnumerateInstanceExtensionProperties(
    nullptr, 0, &count, nullptr);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Getting extension count failed: {}", nextResult);
    return nextResult;
  }

  if (count == 0) {
    DebugPrint(
      "Runtime supports no extensions, so definitely doesn't support hand "
      "tracking. Reporting success but doing nothing.");
    return XR_SUCCESS;
  }

  std::vector<XrExtensionProperties> extensions(count);
  nextResult = gNext->xrEnumerateInstanceExtensionProperties(
    nullptr, count, &count, extensions.data());
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Enumerating extensions failed: {}", nextResult);
    return nextResult;
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
    return XR_SUCCESS;
  }

  if (!foundHandTrackingAim) {
    DebugPrint(
      "Did not find {}, doing nothing.",
      XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
    return XR_SUCCESS;
  }

  if (gInstance) {
    DebugPrint("Already have an instance, refusing to initialize twice");
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  gInstance = new APILayer(*session, gNext);

  return XR_SUCCESS;
}

XrResult xrDestroySession(XrSession session) {
  delete gInstance;
  gInstance = nullptr;
  return gNext->xrDestroySession(session);
}

XrResult xrDestroyInstance(XrInstance instance) {
  delete gInstance;
  gInstance = nullptr;
  return gNext->xrDestroyInstance(instance);
}

XrResult xrGetInstanceProcAddr(
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

XrResult xrCreateApiLayerInstance(
  const XrInstanceCreateInfo* info,
  const struct XrApiLayerCreateInfo* layerInfo,
  XrInstance* instance) {
  DebugPrint("{}", __FUNCTION__);
  //  TODO: check version fields etc in layerInfo
  XrApiLayerCreateInfo nextLayerInfo = *layerInfo;
  nextLayerInfo.nextInfo = layerInfo->nextInfo->next;
  auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
    info, &nextLayerInfo, instance);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Next failed.");
    return nextResult;
  }

  gNext = std::make_shared<OpenXRNext>(
    *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);

  DebugPrint("Created API layer instance");

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
