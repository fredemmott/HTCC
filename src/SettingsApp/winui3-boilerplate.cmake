set_property(
  TARGET HTCCSettings
  PROPERTY VS_PACKAGE_REFERENCES
  "Microsoft.Windows.CppWinRT_${CPPWINRT_VERSION}"
  "Microsoft.WindowsAppSDK_${WINDOWSAPPSDK_VERSION}"
  "Microsoft.Windows.SDK.BuildTools_${WINDOWS_SDK_BUILDTOOLS_VERSION}"
  "Microsoft.Windows.ImplementationLibrary_${WINDOWS_IMPLEMENTATIONLIBRARY_VERSION}"
)

set_target_properties(
  HTCCSettings
  PROPERTIES

  # ----- C++/WinRT, Windows App SDK, and WinUI stuff starts here -----
  VS_GLOBAL_RootNamespace HTCCSettings
  VS_GLOBAL_AppContainerApplication false
  VS_GLOBAL_AppxPackage false
  VS_GLOBAL_CppWinRTOptimized true
  VS_GLOBAL_CppWinRTRootNamespaceAutoMerge true
  VS_GLOBAL_UseWinUI true
  VS_GLOBAL_ApplicationType "Windows Store"
  VS_GLOBAL_ApplicationTypeRevision 10.0
  VS_GLOBAL_WindowsTargetPlatformVersion 10.0
  VS_GLOBAL_WindowsTargetPlatformMinVersion ${MINIMUM_WINDOWS_VERSION}
  VS_GLOBAL_WindowsPackageType None
  VS_GLOBAL_EnablePreviewMsixTooling true
  VS_GLOBAL_WindowsAppSDKSelfContained true
  VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_OnNoMatch_ShowUI true
  VS_GLOBAL_WindowsAppSDKBootstrapAutoInitializeOptions_OnPackageIdentity_NoOp true
)
return()

# Set source file dependencies properly for Xaml and non-Xaml IDL
# files.
#
# Without this, `module.g.cpp` will not include the necessary headers
# for non-Xaml IDL files, e.g. value converters
get_target_property(SOURCES HTCCSettings SOURCES)

foreach(SOURCE ${SOURCES})
  cmake_path(GET SOURCE EXTENSION LAST_ONLY EXTENSION)

  if(NOT "${EXTENSION}" STREQUAL ".idl")
    continue()
  endif()

  set(IDL_SOURCE "${SOURCE}")

  cmake_path(REMOVE_EXTENSION SOURCE LAST_ONLY OUTPUT_VARIABLE BASENAME)
  set(XAML_SOURCE "${BASENAME}.xaml")

  if("${XAML_SOURCE}" IN_LIST SOURCES)
    set_property(
      SOURCE "${IDL_SOURCE}"
      PROPERTY VS_SETTINGS
      "SubType=Code"
      "DependentUpon=${XAML_SOURCE}"
    )
  else()
    set_property(
      SOURCE "${IDL_SOURCE}"
      PROPERTY VS_SETTINGS
      "SubType=Code"
    )
    set_property(
      SOURCE "${BASENAME}.h"
      PROPERTY VS_SETTINGS
      "DependentUpon=${IDL_SOURCE}"
    )
    set_property(
      SOURCE "${BASENAME}.cpp"
      PROPERTY VS_SETTINGS
      "DependentUpon=${IDL_SOURCE}"
    )
  endif()
endforeach()
