add_executable(
  HTCCSettings
  WIN32
  HTCCSettingsApp.manifest
  HTCCSettingsApp.cpp
)
target_include_directories(
  HTCCSettings
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  HTCCSettings
  PRIVATE
  HTCCLibCommon
  HTCCLibPointCtrl
  System::D3D11
  System::Shell32
  System::User32
  fredemmott-gui::fredemmott-gui
)
target_link_options(HTCCSettings PRIVATE "/MANIFESTUAC:level='requireAdministrator'")

set(VERSION_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")
add_custom_command(
  OUTPUT "${VERSION_CPP_FILE}"
  COMMAND
  "${CMAKE_COMMAND}"
  "-DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in"
  "-DOUTPUT_FILE=${VERSION_CPP_FILE}"
  "-DVERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}"
  "-DVERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}"
  "-DVERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}"
  "-DVERSION_BUILD=${CMAKE_PROJECT_VERSION_TWEAK}"
  "-DACTIVE_BUILD_MODE=$<CONFIG>"
  "-DPRODUCT_NAME=${CMAKE_PROJECT_NAME}"
  "-P${CMAKE_SOURCE_DIR}/src/version.cmake"
  DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in"
  "${CMAKE_SOURCE_DIR}/src/version.cmake"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_sources(HTCCSettings PRIVATE "${VERSION_CPP_FILE}")

install(TARGETS HTCCSettings)
install(
  FILES
  "$<TARGET_PDB_FILE:HTCCSettings>"
  DESTINATION "."
  COMPONENT DebugSymbols
)
