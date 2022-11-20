add_library(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  MODULE
  APILayer.cpp
  APILayer_loader.cpp
  DebugPrint.cpp
  OpenXRNext.cpp
  VirtualTouchScreen.cpp
)

target_link_libraries(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  PRIVATE
  ThirdParty::CppWinRT
  ThirdParty::OpenXR
)

target_precompile_headers(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  PRIVATE
  pch.h
)

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking>/XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking.json"
  INPUT "APILayer.json.in"
  NEWLINE_STYLE UNIX
)
