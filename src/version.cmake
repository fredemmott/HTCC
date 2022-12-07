# Need a tag first
# execute_process(
# COMMAND git describe --tags --abbrev=0 HEAD
# OUTPUT_VARIABLE LATEST_GIT_TAG
# OUTPUT_STRIP_TRAILING_WHITESPACE
# WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
# )
set(LATEST_GIT_TAG "NO_TAGS")

if("${VERSION_PATCH}" STREQUAL "0")
  string(REGEX MATCH "^v${VERSION_MAJOR}\\.${VERSION_MINOR}(\\.0|-|$)" MATCHING_TAG "${LATEST_GIT_TAG}")
else()
  set(MATCHING_TAG OFF)
endif()

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(IS_GITHUB_ACTIONS_BUILD "true")
  set(BUILD_TYPE "gha")
else()
  set(IS_GITHUB_ACTIONS_BUILD "false")
  set(BUILD_TYPE "local")
endif()

if("${RELEASE_NAME}" STREQUAL "")
  if(MATCHING_TAG)
    set(VERSION_SEMVER "${LATEST_GIT_TAG}+${BUILD_TYPE}.build.${VERSION_BUILD}")
  else()
    set(VERSION_SEMVER "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}+${BUILD_TYPE}.build.${VERSION_BUILD}")
  endif()
endif()

configure_file(
  ${INPUT_FILE}
  ${OUTPUT_FILE}
  @ONLY
)
