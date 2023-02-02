macro(add_signed_script TARGET SOURCE)
  add_custom_target(
    ${TARGET}
    ALL
    COMMAND
    "${CMAKE_COMMAND}"
    -E
    copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}"
    "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}"
    SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}"
  )

  sign_target_file(
    ${TARGET}
    "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}"
  )

  install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${SOURCE}"
    DESTINATION scripts
    COMPONENT Default
  )
endmacro()

add_signed_script(LayersScript list-openxr-layers.ps1)
