include(ExternalProject)

ExternalProject_Add(
  OpenXRSDKSource
  URL "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.0.26.zip"
  URL_HASH "SHA256=0932da91f0729e4f3a8adb37ec4926d9e5f5abfd771ceb34714c37868f1de6b6"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  EXCLUDE_FROM_ALL
)

ExternalProject_Get_property(OpenXRSDKSource SOURCE_DIR)

add_library(OpenXRSDK INTERFACE)
add_dependencies(OpenXRSDK OpenXRSDKSource)
target_include_directories(
  OpenXRSDK
  INTERFACE
  "${SOURCE_DIR}/src/common"
  "${SOURCE_DIR}/include"
)
add_library(ThirdParty::OpenXR ALIAS OpenXRSDK)

install(
  FILES "${SOURCE_DIR}/LICENSE"
  TYPE DOC
  RENAME "LICENSE-ThirdParty-OpenXR SDK.txt"
)

ExternalProject_Add(
  OpenXRLoaderSource
  URL "https://github.com/KhronosGroup/OpenXR-SDK/releases/download/release-1.0.26/OpenXR.Loader.1.0.26.nupkg"
  URL_HASH "SHA256=ae874fd20e77abc2de4bc8a75ccc9a1217799a6e81cf76eee6a1cd599de45a93"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  EXCLUDE_FROM_ALL
)

ExternalProject_Get_property(OpenXRLoaderSource SOURCE_DIR)

add_library(OpenXRLoader SHARED IMPORTED GLOBAL)
add_dependencies(OpenXRLoader OpenXRLoaderSource)
set_target_properties(
  OpenXRLoader
  PROPERTIES
  IMPORTED_LOCATION "${SOURCE_DIR}/native/x64/release/bin/openxr_loader.dll"
  IMPORTED_IMPLIB "${SOURCE_DIR}/native/x64/release/lib/openxr_loader.lib"
)

add_library(ThirdParty::OpenXRLoader ALIAS OpenXRLoader)
