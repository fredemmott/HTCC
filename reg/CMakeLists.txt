# For NSIS

install(
  FILES
  "DCS.reg"
  "MSFS.reg"
  DESTINATION "reg/"
)

# Convert the .reg files to the Windows Installer's source format
find_program(
  HEAT_EXE
  heat.exe
  DOC "Path to heat.exe from the WiX toolset"
  PATHS
  "${WIX_ROOT_DIR}/bin"
  "$ENV{WIX}/bin"
  "$ENV{WIX_ROOT_DIR}/bin"
)

if("${HEAT_EXE}" STREQUAL "HEAT_EXE-NOTFOUND")
  message(WARNING "Couldn't find heat.exe from the WiX toolset; building an MSI will fail")
  return()
endif()

macro(add_reg_wxs TARGET SOURCE)
  set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}.wxs")
  add_custom_target(
    "${TARGET}"
    ALL
    COMMAND
    "${HEAT_EXE}"
    reg "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}"
    -cg "${TARGET}" # ComponentGroup
    -gg # Generate GUIDs
    -out "${OUTPUT}"
    SOURCES "${SOURCE}"
  )
  set("${TARGET}_PATH" "${OUTPUT}" PARENT_SCOPE)
endmacro()

add_reg_wxs(DCSRegWXS DCS.reg)
add_reg_wxs(MSFSRegWXS MSFS.reg)
