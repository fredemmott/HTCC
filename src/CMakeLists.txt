add_library(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  MODULE
  APILayer.cpp
  OpenXRNext.cpp
)

target_link_libraries(
  XR_APILAYER_FREDEMMOTT_DCSQuestHandTracking
  PRIVATE
  ThirdParty::OpenXR
)
