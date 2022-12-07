install(
  FILES
  "WiX-LICENSE.txt"
  TYPE DOC
  RENAME "LICENSE-ThirdParty-WiX.txt"
  COMPONENT Installer
)

# #### CPack (MSI) Config #####
set(CPACK_GENERATOR "WIX")
set(CPACK_PACKAGE_NAME "Hand Tracked Cockpit Clicking")
set(CPACK_PACKAGE_VENDOR "Fred Emmott")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_WIX_UPGRADE_GUID "2f0cd440-8d59-4572-aabe-a7b4e7ffcdcd")
set(CPACK_WIX_UI_REF "WixUI_Minimal_NoEULA")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME} v${CMAKE_PROJECT_VERSION}")
set(
  CPACK_PACKAGE_EXECUTABLES
  "HTCCSettings;HTCC Settings"
  "PointCtrlCalibration;HTCC PointCTRL Calibration"
)

get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL DebugSymbols)

list(
  APPEND
  CPACK_WIX_EXTRA_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/LaunchAfterInstall.wxs"
  "${CMAKE_CURRENT_SOURCE_DIR}/OpenXRAPILayerRegistration.wxs"
  "${CMAKE_CURRENT_SOURCE_DIR}/WixUI_Minimal_NoEULA.wxs"
  "${DCSRegWXS_PATH}"
  "${MSFSRegWXS_PATH}"
)
list(
  APPEND
  CPACK_WIX_PATCH_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/patch.xml"
)
list(
  APPEND
  CPACK_WIX_EXTENSIONS
  "WixUtilExtension"
)

include(CPack)
cpack_add_component(
  Default
  HIDDEN
)
