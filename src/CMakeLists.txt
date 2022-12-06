function(add_version_metadata TARGET)
  set(VERSION_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc")
  add_custom_command(
    OUTPUT "${VERSION_RC_FILE}"
    COMMAND
    "${CMAKE_COMMAND}"
    "-DINPUT_RC_FILE=${CMAKE_SOURCE_DIR}/src/version.rc.in"
    "-DOUTPUT_RC_FILE=${VERSION_RC_FILE}"
    "-DVERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}"
    "-DVERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}"
    "-DVERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}"
    "-DVERSION_BUILD=${CMAKE_PROJECT_VERSION_TWEAK}"
    "-DACTIVE_BUILD_MODE=$<CONFIG>"
    "-DORIGINAL_FILENAME=$<TARGET_FILE_NAME:${TARGET}>"
    "-DPRODUCT_NAME=${CMAKE_PROJECT_NAME}"
    "-P${CMAKE_SOURCE_DIR}/src/version.cmake"
    DEPENDS
    "${CMAKE_SOURCE_DIR}/src/version.rc.in"
    "${CMAKE_SOURCE_DIR}/src/version.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_sources("${TARGET}" PRIVATE "${VERSION_RC_FILE}")
endfunction()

add_subdirectory(APILayer)
add_subdirectory(lib)
add_subdirectory(PointCtrlCalibration)
