add_signed_script(LayersScript list-openxr-layers.ps1
  DESTINATION scripts
  COMPONENT Default)
install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/HTCC.wprp"
  DESTINATION scripts
  COMPONENT Default)