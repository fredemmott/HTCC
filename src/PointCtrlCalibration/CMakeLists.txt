add_executable(
  PointCtrlCalibration
  WIN32
  PointCtrlCalibration.cpp
)
add_version_metadata(PointCtrlCalibration)

find_package(OpenXR CONFIG REQUIRED)

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
  System::WindowsApp
  Microsoft::DirectXTK
  OpenXR::headers
  OpenXR::openxr_loader
)

target_link_options(PointCtrlCalibration PRIVATE "/MANIFESTUAC:level='requireAdministrator'")