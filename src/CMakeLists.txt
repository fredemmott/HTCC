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

include(sourcelink)

add_subdirectory(APILayer)
add_subdirectory(lib)
add_subdirectory(PointCtrlCalibration)
add_subdirectory(SettingsApp)
