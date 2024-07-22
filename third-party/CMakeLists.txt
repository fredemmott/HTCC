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

include(cppwinrt.cmake)
include(directxtk.cmake)
include(openxr.cmake)
include(nuget-versions.cmake)

return(
  PROPAGATE
  CPPWINRT_VERSION
  WINDOWSAPPSDK_VERSION
  WINDOWS_SDK_BUILDTOOLS_VERSION
  WINDOWS_IMPLEMENTATIONLIBRARY_VERSION
)