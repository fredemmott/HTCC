set(BUILD_SHARED_LIBS OFF)

set(
  THIRDPARTY_TOOLCHAIN_FILE
  "${CMAKE_CURRENT_BINARY_DIR}/toolchain.cmake"
)
configure_file(
  toolchain.cmake.in
  "${THIRDPARTY_TOOLCHAIN_FILE}"
  @ONLY
)

include(system.cmake)

include(openxr.cmake)

# From vcpkg
find_package(cppwinrt CONFIG REQUIRED GLOBAL)
find_package(directxtk CONFIG REQUIRED GLOBAL)
find_package(imgui CONFIG REQUIRED GLOBAL)
find_package(wil CONFIG REQUIRED GLOBAL)
