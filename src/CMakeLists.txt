function(add_version_metadata TARGET)
  configure_file(
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/version.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc.configured"
  )
  file(
    GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc.configured"
  )
  target_sources(
    "${TARGET}"
    PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc"
  )
endfunction()

set(SOURCELINK "" CACHE STRING "URL to retrieve the source")
if (SOURCELINK)
  message(STATUS "Using SOURCELINK: ${SOURCELINK}")
  cmake_path(
    NATIVE_PATH
    CMAKE_SOURCE_DIR
    NORMALIZE
    NATIVE_SOURCE
  )
  string(REPLACE "\\" "\\\\" NATIVE_SOURCE "${NATIVE_SOURCE}")
  file(
    WRITE
    "${CMAKE_CURRENT_BINARY_DIR}/sourcelink.json"
    "{ \"documents\": { \"${NATIVE_SOURCE}\\\\*\": \"${SOURCELINK}/*\" } }"
  )
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_link_options("/SOURCELINK:${CMAKE_CURRENT_BINARY_DIR}/sourcelink.json")
  else ()
    message(WARNING "SOURCELINK was specified, but not using MSVC")
  endif ()
endif ()

add_subdirectory(APILayer)
add_subdirectory(lib)
add_subdirectory(PointCtrlCalibration)
add_subdirectory(PointCtrlClassic)
add_subdirectory(SettingsApp)
