add_library(
  HTCCAPILayer
  MODULE
  APILayer_loader.cpp
  APILayer.cpp
  HandTrackingSource.cpp
  VirtualControllerSink.cpp
  VirtualTouchScreenSink.cpp
)
set_target_properties(
  HTCCAPILayer
  PROPERTIES
  OUTPUT_NAME XR_APILAYER_FREDEMMOTT_HandTrackedCockpitClicking
)
add_version_metadata(HTCCAPILayer)
target_link_libraries(
  HTCCAPILayer
  PRIVATE
  ThirdParty::DirectXTK
  HTCCLibCommon
  HTCCLibPointCtrl
)
install(
  TARGETS
  HTCCAPILayer
  LIBRARY DESTINATION "."
)

install(
  FILES
  "$<TARGET_PDB_FILE:HTCCAPILayer>"
  COMPONENT DebugSymbols
  DESTINATION "."
)

set(API_LAYER_JSON_FILE "$<TARGET_FILE_DIR:HTCCAPILayer>/APILayer.json")
file(
  GENERATE
  OUTPUT "${API_LAYER_JSON_FILE}"
  INPUT "APILayer.json.in"
  NEWLINE_STYLE UNIX
)
install(
  FILES
  "${API_LAYER_JSON_FILE}"
  DESTINATION "."
)
