// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT

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

template <class F, auto Next, auto Layer>
struct XRFuncDelegator;

template <class TRet, class... TArgs, auto Next, auto Layer>
struct XRFuncDelegator<TRet (*)(TArgs...), Next, Layer> {
  static TRet Invoke(TArgs... args) noexcept {
    if (Config::Enabled && gInstance) {
      return std::invoke(Layer, gInstance, args...);
    }
    auto& next = gNext.get()->*Next;
    return std::invoke(next, args...);
  }
};

static XrResult xrDestroyInstance(XrInstance instance) {
  delete gInstance;
  gInstance = nullptr;
  const auto result = gNext->xrDestroyInstance(instance);
  gNext = nullptr;
  return result;
}

static XrResult xrEnumerateInstanceExtensionProperties(
  const char* layerName,
  uint32_t propertyCapacityInput,
  uint32_t* propertyCountOutput,
  XrExtensionProperties* properties) {
  // This is installed as an implicit layer; for behavior spec, see:
  // https://registry.khronos.org/OpenXR/specs/1.0/loader.html#api-layer-conventions-and-rules
  if (layerName && std::string_view {layerName} == OpenXRLayerName) {
    *propertyCountOutput = 0;
    // We don't implement any instance extensions
    return XR_SUCCESS;
  }

  // As we don't implement any extensions, just delegate to the runtime or next
  // layer.
  if (gNext) {
    // We *could* strip the hand tracking extensions here, but we're not
    // - applications wouldn't see this, as the loader just uses the manifest
    // files instead
    // - we can just make the extension functions report failure
    // - probably best to have consistent behavior for applications and API
    // layers
    return gNext->xrEnumerateInstanceExtensionProperties(
      layerName, propertyCapacityInput, propertyCountOutput, properties);
  }

  if (layerName) {
    // If layerName is non-null and not our layer, it should be an earlier
    // layer, or we should have a `next`
    return XR_ERROR_API_LAYER_NOT_PRESENT;
  }

  // for NULL layerName, we append our list to the next; as we have none, that
  // just means we have 0 again
  *propertyCountOutput = 0;
  return XR_SUCCESS;
}

static XrResult xrEnumerateApiLayerProperties(
  uint32_t propertyCapacityInput,
  uint32_t* propertyCountOutput,
  XrApiLayerProperties* properties) {
  // We only return our own properties per spec:
  // https://registry.khronos.org/OpenXR/specs/1.0/loader.html#api-layer-conventions-and-rules
  //
  // TODO: follow-up on "is the spec wrong?" in the Khronos discord - posted at
  // https://discord.com/channels/1044671358782681128/1044672025752514640/1264924075105456199
  //
  // If so, we need to check `gNext`
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
  SPECIAL_INTERCEPTED_OPENXR_FUNCS
#undef IT

#define IT(x) \
  if (name == #x) { \
    *function = reinterpret_cast<PFN_xrVoidFunction>( \
      &XRFuncDelegator<PFN_##x, &OpenXRNext::x, &APILayer::x>::Invoke); \
    return XR_SUCCESS; \
  }
#define IT_EXT(ext, fun) \
  if (name == #fun && !Environment::App_Enabled_##ext) { \
    return XR_ERROR_FUNCTION_UNSUPPORTED; \
  } \
  IT(fun)
  INTERCEPTED_OPENXR_FUNCS
#undef IT
#undef IT_EXT

#define IT(fun)
#define IT_EXT(ext, fun) \
  if (name == #fun && !Environment::App_Enabled_##ext) { \
    return XR_ERROR_FUNCTION_UNSUPPORTED; \
  }
  NEXT_OPENXR_FUNCS;
#undef IT
#undef IT_EXT

  if (gNext) {
    const auto result
      = gNext->xrGetInstanceProcAddr(instance, name_cstr, function);
    if (result != XR_SUCCESS && Config::VerboseDebug >= 1) {
      DebugPrint(
        "xrGetInstanceProcAddr for instance {:#016x} failed: {}",
        reinterpret_cast<uintptr_t>(instance),
        name_cstr);
    }
    return result;
  }

  DebugPrint(
    "Unsupported OpenXR call '{}' with instance {:#016x} and no next",
    name,
    reinterpret_cast<uintptr_t>(instance));
  return XR_ERROR_FUNCTION_UNSUPPORTED;
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

  XrInstanceCreateInfo info {XR_TYPE_INSTANCE_CREATE_INFO};
  if (originalInfo) {
    info = *originalInfo;
    for (uint32_t i = 0; i < info.enabledExtensionCount; ++i) {
      const std::string_view ext {info.enabledExtensionNames[i]};
      if (ext == XR_EXT_HAND_TRACKING_EXTENSION_NAME) {
        Environment::App_Enabled_XR_EXT_hand_tracking = true;
      }
      if (ext == XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME) {
        Environment::App_Enabled_XR_KHR_win32_convert_performance_counter_time
          = true;
      }
    }
  }
  XrApiLayerCreateInfo nextLayerInfo = *layerInfo;
  nextLayerInfo.nextInfo = layerInfo->nextInfo->next;

  if (!Config::Enabled) {
    const auto result = layerInfo->nextInfo->nextCreateApiLayerInstance(
      &info, &nextLayerInfo, instance);
    if (XR_SUCCEEDED(result)) {
      DebugPrint("Created passthru instance as disabled by config");
      gNext = std::make_shared<OpenXRNext>(
        *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);
    }
    return result;
  }

  std::vector<const char*> enabledExtensions;
  for (auto i = 0; i < originalInfo->enabledExtensionCount; ++i) {
    enabledExtensions.push_back(originalInfo->enabledExtensionNames[i]);
  }

  enabledExtensions.push_back(
    XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME);
  enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
  enabledExtensions.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);

  {
    auto last = std::unique(enabledExtensions.begin(), enabledExtensions.end());
    enabledExtensions.erase(last, enabledExtensions.end());
  }

  DebugPrint("Requesting extensions:");
  for (const auto& ext: enabledExtensions) {
    DebugPrint("- {}", ext);
  }

  info.enabledExtensionCount = enabledExtensions.size();
  info.enabledExtensionNames = enabledExtensions.data();

  //// Attempt 1: all 3 extensions
  {
    const auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
      &info, &nextLayerInfo, instance);
    if (XR_SUCCEEDED(nextResult)) {
      Environment::Have_XR_KHR_win32_convert_performance_counter_time = true;
      Environment::Have_XR_EXT_hand_tracking = true;
      Environment::Have_XR_FB_hand_tracking_aim = true;
      gNext = std::make_shared<OpenXRNext>(
        *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);
      gInstance = new APILayer(*instance, gNext);
      DebugPrint("Initialized with all extensions");
      return nextResult;
    }

    if (nextResult != XR_ERROR_EXTENSION_NOT_PRESENT) {
      DebugPrint("all-in xrCreateApiLayerInstance failed: {}", nextResult);
      return nextResult;
    }
  }

  ///// Attempt 2: without XR_FB_hand_tracking_aim
  enabledExtensions.erase(std::ranges::find_if(enabledExtensions, [](auto it) {
    return std::string_view {it} == XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME;
  }));
  info.enabledExtensionCount = enabledExtensions.size();
  info.enabledExtensionNames = enabledExtensions.data();
  {
    const auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
      &info, &nextLayerInfo, instance);
    if (XR_SUCCEEDED(nextResult)) {
      Environment::Have_XR_KHR_win32_convert_performance_counter_time = true;
      Environment::Have_XR_EXT_hand_tracking = true;
      gNext = std::make_shared<OpenXRNext>(
        *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);
      gInstance = new APILayer(*instance, gNext);
      DebugPrint(
        "Initialized without {}", XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
      return nextResult;
    }

    if (nextResult != XR_ERROR_EXTENSION_NOT_PRESENT) {
      DebugPrint(
        "xrCreateInstance without {} failed: {}",
        XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME,
        nextResult);
      return nextResult;
    }
  }

  ///// Attempt 3: without XR_EXT_hand_tracking
  // This is useful when using HTCC as a PointCtrl driver for MSFS
  //
  // We still need XR_KHR_win32_convert_performance_counter_time
  enabledExtensions.erase(std::ranges::find_if(enabledExtensions, [](auto it) {
    return std::string_view {it} == XR_EXT_HAND_TRACKING_EXTENSION_NAME;
  }));
  info.enabledExtensionCount = enabledExtensions.size();
  info.enabledExtensionNames = enabledExtensions.data();
  {
    const auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
      &info, &nextLayerInfo, instance);
    if (XR_SUCCEEDED(nextResult)) {
      Environment::Have_XR_KHR_win32_convert_performance_counter_time = true;
      gNext = std::make_shared<OpenXRNext>(
        *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);
      gInstance = new APILayer(*instance, gNext);
      DebugPrint("Initialized without {}", XR_EXT_HAND_TRACKING_EXTENSION_NAME);
      return nextResult;
    }

    if (nextResult != XR_ERROR_EXTENSION_NOT_PRESENT) {
      DebugPrint(
        "xrCreateInstance without {} failed: {}",
        XR_EXT_HAND_TRACKING_EXTENSION_NAME,
        nextResult);
      return nextResult;
    }
  }

  ///// Attempt 4: nope, nothing. Just pass through.
  const auto nextResult = layerInfo->nextInfo->nextCreateApiLayerInstance(
    originalInfo, &nextLayerInfo, instance);
  if (XR_SUCCEEDED(nextResult)) {
    DebugPrint("No-op passthrough xrCreateAPILayerInstance succeeded");
    gNext = std::make_shared<OpenXRNext>(
      *instance, layerInfo->nextInfo->nextGetInstanceProcAddr);
  } else {
    DebugPrint(
      "No-op passthrough xrCreateApiLayerInstance failed: {}", nextResult);
  }
  return nextResult;
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
