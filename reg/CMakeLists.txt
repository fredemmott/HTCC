foreach (GAME IN ITEMS DCS MSFS)
  add_custom_command(
    OUTPUT "${OUTPUT_ROOT}/installer/${GAME}.reg"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${GAME}.reg"
    COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/${GAME}.reg"
    "${OUTPUT_ROOT}/installer/${GAME}.reg"
  )
  add_custom_target(
    "settings-${GAME}"
    ALL
    DEPENDS
    "${OUTPUT_ROOT}/installer/${GAME}.reg"
  )
endforeach ()