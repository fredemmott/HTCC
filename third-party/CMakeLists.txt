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

# From vcpkg
find_package(directxtk CONFIG REQUIRED GLOBAL)
find_package(wil CONFIG REQUIRED GLOBAL)

set(fredemmmott-gui_SOURCE_DIR "" CACHE PATH "Path to a local checkout of fredemmott-gui")
if (fredemmott-gui_SOURCE_DIR)
  set(ENABLE_IMPLICIT_BACKENDS OFF)
  set(ENABLE_DIRECT2D ON)
  add_subdirectory(
    "${fredemmott-gui_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/third-party/fredemmott-gui"
    EXCLUDE_FROM_ALL
  )
else ()
  # vcpkg
  find_package(fredemmott-gui CONFIG REQUIRED GLOBAL)
endif ()
