add_custom_target(
  copy-wprp ALL
  COMMAND
  "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:HTCCSettings>"
  COMMAND
  "${CMAKE_COMMAND}" -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/HTCC.wprp"
  "$<TARGET_FILE_DIR:HTCCSettings>/HTCC.wprp"
)
