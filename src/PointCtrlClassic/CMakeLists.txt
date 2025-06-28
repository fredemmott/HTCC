add_executable(
  PointCtrlClassic
  PointCtrlClassic.cpp
)
sign_target(PointCtrlClassic)
add_version_metadata(PointCtrlClassic)

target_link_libraries(
  PointCtrlClassic
  PRIVATE
  HTCCLibCommon
  HTCCLibPointCtrl
  System::WindowsApp
)

install(TARGETS PointCtrlClassic)

install(
  FILES
  "$<TARGET_PDB_FILE:PointCtrlClassic>"
  COMPONENT DebugSymbols
  DESTINATION "."
)
