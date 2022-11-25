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
  VirtualTouchScreen.cpp
)

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
