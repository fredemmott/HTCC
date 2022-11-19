add_library(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  MODULE
  APILayer.cpp
  DebugPrint.cpp
  OpenXRNext.cpp
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
