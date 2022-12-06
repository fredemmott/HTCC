add_executable(
  PointCtrlCalibration
  WIN32
  PointCtrlCalibration.cpp
)
sign_target(PointCtrlCalibration)
add_version_metadata(PointCtrlCalibration)

target_link_libraries(
  PointCtrlCalibration
  PRIVATE
  HTCCLibCommon
  HTCCLibPointCtrl
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

install(TARGETS PointCtrlCalibration)

install(
  FILES
  "$<TARGET_RUNTIME_DLLS:PointCtrlCalibration>"
  DESTINATION "."
)

install(
  FILES
  "$<TARGET_PDB_FILE:PointCtrlCalibration>"
  COMPONENT DebugSymbols
  DESTINATION "."
)
