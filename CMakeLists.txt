cmake_minimum_required(VERSION 3.23)

# No specific need for this version, but let's set *something* so
# the lower bound is known and enforced
set(MINIMUM_WINDOWS_VERSION "10.0.19041.0")
set(CMAKE_SYSTEM_VERSION "${MINIMUM_WINDOWS_VERSION}")

# Required for XAML resources in self-contained mode
set(CMAKE_INSTALL_BINDIR "." CACHE STRING "Path to install binaries relative to the root" FORCE)
set(CMAKE_INSTALL_DOCDIR "docs" CACHE STRING "Path to install documentation" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(

  # Standard C++ exception behavior
  "/EHsc"
)

# Require that targets exist
cmake_policy(SET CMP0079 NEW)
set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS ON)
cmake_policy(SET CMP0028 NEW)

# We're going to use the 'Hybrid CRT' approach, which is the combination of the
# UCRT and the static C++ Runtime
#
# https://github.com/microsoft/WindowsAppSDK/blob/main/docs/Coding-Guidelines/HybridCRT.md

# Enable CMAKE_MSVC_RUNTIME_LIBRARY variable
cmake_policy(SET CMP0091 NEW)
set(
  CMAKE_MSVC_RUNTIME_LIBRARY
  "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

# For backwards-compat with third-party projects that don't have CMP0091 set
set(
  MSVC_RUNTIME_LIBRARY_COMPILE_OPTION
  "/MT$<$<CONFIG:Debug>:d>"
)
set(
  COMMON_LINK_OPTIONS
  "/DEFAULTLIB:ucrt$<$<CONFIG:Debug>:d>.lib" # include the dynamic UCRT
  "/NODEFAULTLIB:libucrt$<$<CONFIG:Debug>:d>.lib" # remove the static UCRT
)
add_link_options("${COMMON_LINK_OPTIONS}")

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
else()
  set(VERSION_BUILD 0)
endif()

project(
  XR_APILAYER_FREDEMMOTT_HandTrackedCockpitClicking
  VERSION 0.1.2.${VERSION_BUILD}
  LANGUAGES CXX C)
message(STATUS "${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}")

# Handy for CI
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${CMAKE_PROJECT_VERSION}")

if(MSVC AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CLANG_CL 1 CACHE BOOL "Whether we're using clang-cl")
else()
  set(CLANG_CL 0)
endif()

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Simulated compiler: ${CMAKE_CXX_SIMULATE_ID}")
message(STATUS "MSVC: ${MSVC}")
message(STATUS "CLANG_CL: ${CLANG_CL}")

set(COMMON_COMPILE_OPTIONS "/DUNICODE" "/D_UNICODE")

if(MSVC AND NOT CLANG_CL)
  list(
    APPEND COMMON_COMPILE_OPTIONS

    # Standards-compliant C++20 coroutines
    "/await:strict"

    # Synchronous writes to PDB files in case building with parallel CL.exe
    "/FS"

    # Include content and marker in error messages
    "/diagnostics:caret"
  )
endif()

add_compile_options(${COMMON_COMPILE_OPTIONS})

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Default)

include(sign_target.cmake)

add_subdirectory("third-party")
add_subdirectory("src")
add_subdirectory("reg")
add_subdirectory("scripts")

install(
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
  TYPE DOC
)
install(
  DIRECTORY docs/
  TYPE DOC
)

add_subdirectory(installer)
