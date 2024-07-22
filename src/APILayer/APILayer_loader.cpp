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
#include <openxr/openxr_platform.h>

#include <filesystem>

#include "APILayer.h"
#include "Config.h"
#include "DebugPrint.h"
#include "Environment.h"
#include "OpenXRNext.h"

namespace Environment = HandTrackedCockpitClicking::Environment;

namespace HandTrackedCockpitClicking::Loader {

constexpr std::string_view OpenXRLayerName {
  "XR_APILAYER_FREDEMMOTT_HandTrackedCockpitClicking"};
static_assert(OpenXRLayerName.size() <= XR_MAX_API_LAYER_NAME_SIZE);

static std::shared_ptr<OpenXRNext> gNext;
static APILayer* gInstance = nullptr;

static XrResult xrGetSystemProperties(
  XrInstance instance,
  XrSystemId systemId,
  XrSystemProperties* properties) {
  if (gInstance) {
    return gInstance->xrGetSystemProperties(instance, systemId, properties);
  }
  return gNext->xrGetSystemProperties(instance, systemId, properties);
}

static XrResult xrCreateHandTrackerEXT(
  XrSession session,
  const XrHandTrackerCreateInfoEXT* createInfo,
  XrHandTrackerEXT* handTracker) {
  if (gInstance) {
    return gInstance->xrCreateHandTrackerEXT(session, createInfo, handTracker);
  }
  return gNext->xrCreateHandTrackerEXT(session, createInfo, handTracker);
}

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

static XrResult xrGetActionStateBoolean(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStateBoolean* state) {
  if (gInstance) {
    return gInstance->xrGetActionStateBoolean(session, getInfo, state);
  }
  return gNext->xrGetActionStateBoolean(session, getInfo, state);
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

static XrResult xrGetActionStatePose(
  XrSession session,
  const XrActionStateGetInfo* getInfo,
  XrActionStatePose* state) {
  if (gInstance) {
    return gInstance->xrGetActionStatePose(session, getInfo, state);
  }
  return gNext->xrGetActionStatePose(session, getInfo, state);
}

static XrResult xrGetCurrentInteractionProfile(
  XrSession session,
  XrPath topLevelUserPath,
  XrInteractionProfileState* interactionProfile) {
  if (gInstance) {
    return gInstance->xrGetCurrentInteractionProfile(
      session, topLevelUserPath, interactionProfile);
  }
  return gNext->xrGetCurrentInteractionProfile(
    session, topLevelUserPath, interactionProfile);
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

static XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
  if (gInstance) {
    return gInstance->xrPollEvent(instance, eventData);
  }
  return gNext->xrPollEvent(instance, eventData);
}

static XrResult xrCreateAction(
  XrActionSet actionSet,
  const XrActionCreateInfo* createInfo,
  XrAction* action) {
  if (gInstance) {
    return gInstance->xrCreateAction(actionSet, createInfo, action);
  }
  return gNext->xrCreateAction(actionSet, createInfo, action);
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
  static uint32_t sCount = 0;
  DebugPrint("{}(): #{}", __FUNCTION__, sCount++);
  auto nextResult = gNext->xrCreateSession(instance, createInfo, session);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Failed to create OpenXR session: {}", nextResult);
    return nextResult;
  }

  if (!Config::Enabled) {
    DebugPrint("Not enabled, doing nothing");
    return XR_SUCCESS;
  }

  DebugPrint("Initializing API layer");
  gInstance = new APILayer(instance, *session, gNext);

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
  const auto result = gNext->xrDestroyInstance(instance);
  gNext = nullptr;
  return result;
}

static XrResult xrEnumerateApiLayerProperties(
  uint32_t propertyCapacityInput,
  uint32_t* propertyCountOutput,
  XrApiLayerProperties* properties) {
  // We only return our own properties per spec:
  // https://registry.khronos.org/OpenXR/specs/1.0/loader.html#api-layer-conventions-and-rules
  *propertyCountOutput = 1;

  if (propertyCapacityInput == 0) {
    // Do not return XR_ERROR_SIZE_SUFFICIENT for 0, per
    // https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#buffer-size-parameters
    return XR_SUCCESS;
  }

  if (properties->type != XR_TYPE_API_LAYER_PROPERTIES) {
    return XR_ERROR_VALIDATION_FAILURE;
  }

  *properties = XrApiLayerProperties {
    .type = XR_TYPE_API_LAYER_PROPERTIES,
    .next = properties->next,
    .layerName = "XR_APILAYER_FREDEMMOTT_HandTrackedCockpitClicking",
    .specVersion = XR_CURRENT_API_VERSION,
    .layerVersion = 1,
    .description
    = "Hand-tracked cockpit clicking for flight simulators - "
      "https://github.com/fredemmott/hand-tracked-cockpit-clicking",
  };

  return XR_SUCCESS;
}

static XrResult xrGetInstanceProcAddr(
  XrInstance instance,
  const char* name_cstr,
  PFN_xrVoidFunction* function) {
  std::string_view name {name_cstr};

#define IT(x) \
  if (name == #x) { \
    *function = reinterpret_cast<PFN_xrVoidFunction>(&x); \
    return XR_SUCCESS; \
  }
  INTERCEPTED_OPENXR_FUNCS
#undef IT

  if (gNext) {
    const auto result
      = gNext->raw_xrGetInstanceProcAddr(instance, name_cstr, function);
    if (result != XR_SUCCESS && Config::VerboseDebug >= 1) {
      DebugPrint(
        "xrGetInstanceProcAddr for instance {:#016x} failed: {}",
        reinterpret_cast<uintptr_t>(instance),
        name_cstr);
    }
    return result;
  }

  if (name == "xrEnumerateApiLayerProperties") {
    *function
      = reinterpret_cast<PFN_xrVoidFunction>(&xrEnumerateApiLayerProperties);
    return XR_SUCCESS;
  }

  DebugPrint(
    "Unsupported OpenXR call '{}' with instance {:#016x} and no next",
    name,
    reinterpret_cast<uintptr_t>(instance));
  return XR_ERROR_FUNCTION_UNSUPPORTED;
}

static void EnumerateExtensions(OpenXRNext* oxr) {
  uint32_t extensionCount = 0;

  auto nextResult = oxr->xrEnumerateInstanceExtensionProperties(
    nullptr, 0, &extensionCount, nullptr);
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Getting extension count failed: {}", nextResult);
    return;
  }

  if (extensionCount == 0) {
    DebugPrint(
      "Runtime supports no extensions, so definitely doesn't support hand "
      "tracking. Reporting success but doing nothing.");
    return;
  }

  std::vector<XrExtensionProperties> extensions(
    extensionCount, XrExtensionProperties {XR_TYPE_EXTENSION_PROPERTIES});
  nextResult = oxr->xrEnumerateInstanceExtensionProperties(
    nullptr, extensionCount, &extensionCount, extensions.data());
  if (nextResult != XR_SUCCESS) {
    DebugPrint("Enumerating extensions failed: {}", nextResult);
    return;
  }

  for (const auto& it: extensions) {
    const std::string_view name {it.extensionName};
    if (Config::VerboseDebug) {
      DebugPrint("Found {}", name);
    }

    if (name == XR_EXT_HAND_TRACKING_EXTENSION_NAME) {
      Environment::Have_XR_EXT_HandTracking = true;
      continue;
    }
    if (
      name == XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME
      && Config::EnableFBOpenXRExtensions) {
      Environment::Have_XR_FB_HandTracking_Aim = true;
      continue;
    }
    if (name == XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME) {
      Environment::Have_XR_KHR_win32_convert_performance_counter_time = true;
      continue;
    }
  }

  DebugPrint(
    "{}: {}",
    XR_EXT_HAND_TRACKING_EXTENSION_NAME,
    Environment::Have_XR_EXT_HandTracking);
  DebugPrint(
    "{}: {}",
    XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME,
    Environment::Have_XR_FB_HandTracking_Aim);
  DebugPrint(
    "{}: {}",
    XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME,
    Environment::Have_XR_KHR_win32_convert_performance_counter_time);
}

static XrResult xrCreateApiLayerInstance(
  const XrInstanceCreateInfo* originalInfo,
  const struct XrApiLayerCreateInfo* layerInfo,
  XrInstance* instance) {
  static uint32_t sCount = {0};
  DebugPrint(
    "{} #{} {:#016x} {:#016x}",
    __FUNCTION__,
    sCount++,
    reinterpret_cast<const uintptr_t>(originalInfo),
    reinterpret_cast<const uintptr_t>(layerInfo));
  if (gNext) {
    DebugPrint("Still have a gNext, wtf?!");
    gNext = nullptr;
  }

  //  TODO: check version fields etc in layerInfo

  XrInstance dummyInstance {};
  {
    // While the OpenXR specification says that
    // `xrEnumerateInstanceExtensionProperties()` does not require an
    // `XrInstance`, if there is a 'next' API layer, it will not be able to
    // retrieve the extension list from the runtime or an n+2 API layer unless
    // an instance has been created, as it won't have the next
    // `xrGetInstanceProcAddr()` pointer yet.
    XrInstanceCreateInfo dummyInfo {
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .applicationInfo = {
        .applicationVersion = originalInfo->applicationInfo.applicationVersion,
        .engineName = "FREDEMMOTT_HTCC",
        .engineVersion = 1,
        .apiVersion = XR_CURRENT_API_VERSION,
      },
    };
    {
      auto [it, count] = std::format_to_n(
        dummyInfo.applicationInfo.applicationName,
        XR_MAX_APPLICATION_NAME_SIZE - 1,
        "FREDEMMOTT_HTCC Init: {}",
        originalInfo->applicationInfo.applicationName);
      *it = '\0';
    }
    auto dummyLayerInfo = *layerInfo;
    dummyLayerInfo.nextInfo = dummyLayerInfo.nextInfo->next;
    layerInfo->nextInfo->nextCreateApiLayerInstance(
      &dummyInfo, &dummyLayerInfo, &dummyInstance);
  }

  OpenXRNext next(dummyInstance, layerInfo->nextInfo->nextGetInstanceProcAddr);

  XrInstanceCreateInfo info {XR_TYPE_INSTANCE_CREATE_INFO};
  if (originalInfo) {
    info = *originalInfo;
  }

  std::vector<const char*> enabledExtensions;
  if (Config::Enabled) {
    EnumerateExtensions(&next);
    if (Environment::Have_XR_EXT_HandTracking) {
      DebugPrint("Original extensions:");
      for (auto i = 0; i < originalInfo->enabledExtensionCount; ++i) {
        DebugPrint("- {}", originalInfo->enabledExtensionNames[i]);
        enabledExtensions.push_back(originalInfo->enabledExtensionNames[i]);
      }

      enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
      // We need 'real time' units to rotate the hand at a known speed for
      // MSFS
      if (Environment::Have_XR_KHR_win32_convert_performance_counter_time) {
        enabledExtensions.push_back(
          XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME);
      }
      // Required for enhanced pinch gestures on Quest
      if (Environment::Have_XR_FB_HandTracking_Aim) {
        enabledExtensions.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
      }

      {
        auto last
          = std::unique(enabledExtensions.begin(), enabledExtensions.end());
        enabledExtensions.erase(last, enabledExtensions.end());
      }

      DebugPrint("Requesting extensions:");
      for (const auto& ext: enabledExtensions) {
        DebugPrint("- {}", ext);
      }

      info.enabledExtensionCount = enabledExtensions.size();
      info.enabledExtensionNames = enabledExtensions.data();
    }
  }
  next.check_xrDestroyInstance(dummyInstance);

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

}// namespace HandTrackedCockpitClicking::Loader

using HandTrackedCockpitClicking::DebugPrint;

extern "C" {
XrResult __declspec(dllexport) XRAPI_CALL
HandTrackedCockpitClicking_xrNegotiateLoaderApiLayerInterface(
  const XrNegotiateLoaderInfo* loaderInfo,
  const char* layerName,
  XrNegotiateApiLayerRequest* apiLayerRequest) {
  if (layerName != HandTrackedCockpitClicking::Loader::OpenXRLayerName) {
    DebugPrint(
      "Layer name mismatch:\n -{}\n +{}",
      HandTrackedCockpitClicking::Loader::OpenXRLayerName,
      layerName);
    return XR_ERROR_INITIALIZATION_FAILED;
  }

  HandTrackedCockpitClicking::Config::LoadForCurrentProcess();
  HandTrackedCockpitClicking::Environment::Load();

  // TODO: check version fields etc in loaderInfo

  apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
  apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
  apiLayerRequest->getInstanceProcAddr
    = &HandTrackedCockpitClicking::Loader::xrGetInstanceProcAddr;
  apiLayerRequest->createApiLayerInstance
    = &HandTrackedCockpitClicking::Loader::xrCreateApiLayerInstance;
  return XR_SUCCESS;
}
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved) {
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      TraceLoggingRegister(HandTrackedCockpitClicking::gTraceProvider);
      break;
    case DLL_PROCESS_DETACH:
      TraceLoggingUnregister(HandTrackedCockpitClicking::gTraceProvider);
      break;
  }
  return TRUE;
}
