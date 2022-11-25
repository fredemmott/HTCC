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

#include <loader_interfaces.h>
#include <openxr/openxr.h>

#include <filesystem>

#include "APILayer.h"
#include "Config.h"
#include "DebugPrint.h"
#include "OpenXRNext.h"

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};

namespace DCSQuestHandTracking::Loader {

constexpr std::string_view OpenXRLayerName {
  "XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking"};
static_assert(OpenXRLayerName.size() <= XR_MAX_API_LAYER_NAME_SIZE);

static std::shared_ptr<OpenXRNext> gNext;
static APILayer* gInstance = nullptr;
static bool gIsDCS = true;
static bool gHaveHandTracking = false;

static XrResult xrWaitFrame(
  XrSession session,
  const XrFrameWaitInfo* info,
  XrFrameState* state) {
  if (gInstance) {
    return gInstance->xrWaitFrame(session, info, state);
  }
  return gNext->xrWaitFrame(session, info, state);
}

static XrResult xrSuggestInteractionProfileBindings(
  XrInstance instance,
  const XrInteractionProfileSuggestedBinding* suggestedBindings) {
  if (gInstance) {
    return gInstance->xrSuggestInteractionProfileBindings(
      instance, suggestedBindings);
  }
  return gNext->xrSuggestInteractionProfileBindings(
    instance, suggestedBindings);
}

static XrResult xrGetActionStateFloat(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateFloat* state) {
  if (gInstance) {
    return gInstance->xrGetActionStateFloat(session, getInfo, state);
  }
  return gNext->xrGetActionStateFloat(session, getInfo, state);
}

static XrResult xrLocateSpace(
  XrSpace space,
  XrSpace baseSpace,
  XrTime time,
  XrSpaceLocation* location) {
  if (gInstance) {
    return gInstance->xrLocateSpace(space, baseSpace, time, location);
  }
  return gNext->xrLocateSpace(space, baseSpace, time, location);
}

static XrResult xrSyncActions(
  XrSession session,
  const XrActionsSyncInfo* syncInfo) {
  if (gInstance) {
    return gInstance->xrSyncActions(session, syncInfo);
  }
  return gNext->xrSyncActions(session, syncInfo);
}

static XrResult xrCreateActionSpace(
  XrSession session,
  const XrActionSpaceCreateInfo* createInfo,
  XrSpace* space) {
  if (gInstance) {
    return gInstance->xrCreateActionSpace(session, createInfo, space);
  }
  return gNext->xrCreateActionSpace(session, createInfo, space);
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

  if (!Config::Enabled) {
    DebugPrint("Disabled, doing nothing");
    return XR_SUCCESS;
  }

  if (!gIsDCS) {
    DebugPrint("Not DCS, doing nothing");
    return XR_SUCCESS;
  }

  DebugPrint("Initializing API layer");
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
  if (name == "xrWaitFrame") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrWaitFrame);
    return XR_SUCCESS;
  }
  if (name == "xrSuggestInteractionProfileBindings") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(
      &xrSuggestInteractionProfileBindings);
    return XR_SUCCESS;
  }
  if (name == "xrCreateActionSpace") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrCreateActionSpace);
    return XR_SUCCESS;
  }
  if (name == "xrGetActionStateFloat") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrGetActionStateFloat);
    return XR_SUCCESS;
  }
  if (name == "xrLocateSpace") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrLocateSpace);
    return XR_SUCCESS;
  }
  if (name == "xrSyncActions") {
    *function = reinterpret_cast<PFN_xrVoidFunction>(&xrSyncActions);
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
    if (Config::VerboseDebug) {
      DebugPrint("Found {}", name);
    }

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
  gHaveHandTracking = HaveHandTracking(&next);
  if (gIsDCS && gHaveHandTracking && Config::Enabled) {
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

}// namespace DCSQuestHandTracking::Loader

using DCSQuestHandTracking::DebugPrint;

extern "C" {
XrResult __declspec(dllexport) XRAPI_CALL
  DCSQuestHandTracking_xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo* loaderInfo,
    const char* layerName,
    XrNegotiateApiLayerRequest* apiLayerRequest) {
  if (layerName != DCSQuestHandTracking::Loader::OpenXRLayerName) {
    DebugPrint(
      "Layer name mismatch:\n -{}\n +{}",
      DCSQuestHandTracking::Loader::OpenXRLayerName,
      layerName);
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  DCSQuestHandTracking::Config::Load();

  wchar_t executablePath[MAX_PATH];
  const auto executablePathLen
    = GetModuleFileNameW(NULL, executablePath, MAX_PATH);
  const auto exeName = std::filesystem::path(
                         std::wstring_view {executablePath, executablePathLen})
                         .filename();
  if (exeName != L"DCS.exe") {
    DebugPrint(L"'{}' is not 'DCS.exe'", exeName.wstring());
    if (!DCSQuestHandTracking::Config::CheckDCS) {
      DebugPrint("Loading anyway, Config::CheckDCS is false");
    } else {
      DebugPrint("Skipping.");
      DCSQuestHandTracking::Loader::gIsDCS = false;
    }
  }

  // TODO: check version fields etc in loaderInfo

  apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
  apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
  apiLayerRequest->getInstanceProcAddr
    = &DCSQuestHandTracking::Loader::xrGetInstanceProcAddr;
  apiLayerRequest->createApiLayerInstance
    = &DCSQuestHandTracking::Loader::xrCreateApiLayerInstance;
  return XR_SUCCESS;
}
}
