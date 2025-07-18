set(OUT_DIR "${OUTPUT_ROOT}/installer")

set(LICENSE_RTF "${OUT_DIR}/LICENSE.rtf")
add_custom_command(
  OUTPUT "${LICENSE_RTF}"
  COMMAND
  powershell
  -ExecutionPolicy Bypass
  "${CMAKE_CURRENT_SOURCE_DIR}/ConvertTo-Rtf.ps1"
  "${CMAKE_SOURCE_DIR}/LICENSE"
  "${LICENSE_RTF}"
  MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/LICENSE"
  DEPENDS ConvertTo-Rtf.ps1
)
add_custom_target(license-rtf ALL SOURCES "${LICENSE_RTF}")

set(IS_TAGGED_BUILD_JSON "false")
set(IS_STABLE_RELEASE_JSON "false")
if (IS_TAGGED_BUILD)
  set(IS_TAGGED_BUILD_JSON "true")
endif ()
if (IS_STABLE_RELEASE)
  set(IS_STABLE_RELEASE_JSON "false")
endif ()
set(VERSION_JSON "${CMAKE_CURRENT_BINARY_DIR}/version.json")

configure_file(
  version.json.in
  "${CMAKE_CURRENT_BINARY_DIR}/version.json.configured"
)
file(
  GENERATE
  OUTPUT "${OUT_DIR}/version.json"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/version.json.configured"
)
file(
  GENERATE
  OUTPUT "${OUT_DIR}/version.txt"
  CONTENT "${VERSION_SEMVER}"
)

file(
  DOWNLOAD
  "https://github.com/fredemmott/autoupdates/releases/download/vicius-v1.8.876%2Bfredemmott.2/Updater-Release.exe"
  "${CMAKE_CURRENT_BINARY_DIR}/updater.exe"
  EXPECTED_HASH "SHA256=d0cadc82c17ca8d39987837e9d89b7cb3079437353ecf081e579abc14ecce317"
)
set(UPDATER_EXE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fredemmott_HTCC_Updater.exe")
add_custom_command(
  OUTPUT "${UPDATER_EXE}"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/updater.exe"
  COMMAND
  "${CMAKE_COMMAND}" -E copy_if_different
  "${CMAKE_CURRENT_BINARY_DIR}/updater.exe" "${UPDATER_EXE}"
)
add_custom_target(updater ALL DEPENDS "${UPDATER_EXE}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/updater.json.in"
  "${CMAKE_CURRENT_BINARY_DIR}/updater.json.configured"
)
file(
  GENERATE
  OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fredemmott_HTCC_Updater.json"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/updater.json.configured"
)