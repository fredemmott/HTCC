function(add_version_metadata TARGET)
  set(VERSION_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-version.rc")
  add_custom_command(
    OUTPUT "${VERSION_RC_FILE}"
    COMMAND
    "${CMAKE_COMMAND}"
    "-DINPUT_RC_FILE=${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in"
    "-DOUTPUT_RC_FILE=${VERSION_RC_FILE}"
    "-DVERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}"
    "-DVERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}"
    "-DVERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}"
    "-DVERSION_BUILD=${CMAKE_PROJECT_VERSION_TWEAK}"
    "-DACTIVE_BUILD_MODE=$<CONFIG>"
    "-DORIGINAL_FILENAME=$<TARGET_FILE_NAME:${TARGET}>"
    "-DPRODUCT_NAME=${CMAKE_PROJECT_NAME}"
    "-P${CMAKE_CURRENT_SOURCE_DIR}/version.cmake"
    DEPENDS version.rc.in version.cmake
  )
  target_sources("${TARGET}" PRIVATE "${VERSION_RC_FILE}")
endfunction()

add_library(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  MODULE
  APILayer.cpp
  APILayer_loader.cpp
  Config.cpp
  DebugPrint.cpp
  HandTrackingSource.cpp
  OpenXRNext.cpp
  PointCtrlSource.cpp
  VirtualControllerSink.cpp
  VirtualTouchScreenSink.cpp
)
add_version_metadata(XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking)

target_link_libraries(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  PRIVATE
  System::Dinput8
  System::Dxguid
  ThirdParty::CppWinRT
  ThirdParty::OpenXR
)

target_precompile_headers(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  PRIVATE
  pch.h
)

add_executable(
  PointCtrlCalibration
  WIN32
  Config.cpp
  DebugPrint.cpp
  PointCtrlCalibration.cpp
  PointCtrlSource.cpp
)
add_version_metadata(PointCtrlCalibration)

target_precompile_headers(
  PointCtrlCalibration
  PRIVATE
  pch.h
)

target_link_libraries(
  PointCtrlCalibration
  PRIVATE
  System::D2D1
  System::D3D11
  System::Dinput8
  System::DWrite
  System::Dxguid
  System::DXGI
  ThirdParty::CppWinRT
  ThirdParty::DirectXTK
  ThirdParty::OpenXR
  ThirdParty::OpenXRLoader
)

target_link_options(PointCtrlCalibration PRIVATE "/MANIFESTUAC:level='requireAdministrator'")

add_custom_command(
  TARGET PointCtrlCalibration POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
  "$<TARGET_RUNTIME_DLLS:PointCtrlCalibration>"
  "$<TARGET_FILE_DIR:PointCtrlCalibration>"
)

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking>/XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking.json"
  INPUT "APILayer.json.in"
  NEWLINE_STYLE UNIX
)
