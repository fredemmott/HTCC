add_executable(
  HTCCSettings
  WIN32
  HTCCSettingsApp.manifest
  HTCCSettingsApp.cpp
)
target_include_directories(
  HTCCSettings
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  HTCCSettings
  PRIVATE
  HTCCLibCommon
  HTCCLibPointCtrl
  System::D3D11
  System::Shell32
  System::User32
  fredemmott-gui::fredemmott-gui
)
target_link_options(HTCCSettings PRIVATE "/MANIFESTUAC:level='requireAdministrator'")

set(VERSION_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/include/version.h")
configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h.configured")
file(
  GENERATE
  OUTPUT "${VERSION_HEADER_FILE}"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/version.h.configured"
)
target_sources(HTCCSettings PRIVATE "${VERSION_HEADER_FILE}")
target_include_directories(
  HTCCSettings
  PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/include"
)

find_package(compressed-embed CONFIG REQUIRED)
include(CompressedEmbed)
add_compressed_embed_library(
  licenses
  CLASSNAME Licenses
  OUTPUT_CPP "${CMAKE_CURRENT_BINARY_DIR}/Licenses.cpp"
  OUTPUT_HPP "${CMAKE_CURRENT_BINARY_DIR}/include/Licenses.hpp"
  NAMESPACE "HandTrackedCockpitClicking"
  INPUTS
  Self "${PROJECT_SOURCE_DIR}/LICENSE"
  CompressedEmbed "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/compressed-embed/copyright"
  DirectXMath "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/directxmath/copyright"
  DirectXTK "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/directxtk/copyright"
  FUI "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/fredemmott-gui/copyright"
  OpenXR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/openxr-loader/copyright"
  WIL "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/wil/copyright"
  Yoga "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/yoga/copyright"
)
target_include_directories(licenses PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(HTCCSettings PRIVATE licenses)

install(TARGETS HTCCSettings)
install(
  FILES
  "$<TARGET_PDB_FILE:HTCCSettings>"
  DESTINATION "."
  COMPONENT DebugSymbols
)
