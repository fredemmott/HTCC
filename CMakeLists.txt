cmake_minimum_required(VERSION 3.25...3.31 FATAL_ERROR)

# No specific need for this version, but let's set *something* so
# the lower bound is known and enforced
set(MINIMUM_WINDOWS_VERSION "10.0.19041.0")
set(CMAKE_SYSTEM_VERSION "${MINIMUM_WINDOWS_VERSION}")

# Required for XAML resources in self-contained mode
set(CMAKE_INSTALL_BINDIR "." CACHE STRING "Path to install binaries relative to the root" FORCE)
set(CMAKE_INSTALL_DOCDIR "docs" CACHE STRING "Path to install documentation" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
  # Standard C++ exception behavior
  "/EHsc"
  # UTF-8 sources
  "/utf-8"
)

# Require that targets exist
cmake_policy(SET CMP0079 NEW)
set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS ON)
cmake_policy(SET CMP0028 NEW)

# Set extracted file timestamp to extract time
cmake_policy(SET CMP0135 NEW)

set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)

include(cmake/hybrid-crt.cmake)

set(
  CMAKE_TOOLCHAIN_FILE
  "${CMAKE_SOURCE_DIR}/third-party/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file"
)

set(VERSION_TWEAK 0 CACHE STRING "Final component of build number")
project(
  HTCC
  VERSION 1.4.0.${VERSION_TWEAK}
  LANGUAGES CXX C)

if (NOT VERSION_TWEAK_LABEL)
  set(VERSION_TWEAK_LABEL "local")
endif ()
option(IS_TAGGED_BUILD "Whether or not we're building a tagged version" OFF)

math(EXPR MINOR_VER_MOD_2 "${PROJECT_VERSION_MINOR} % 2")
math(EXPR PATCH_VER_MOD_2 "${PROJECT_VERSION_PATCH} % 2")
if (IS_TAGGED_BUILD AND MINOR_VER_MOD_2 EQUAL 0 AND PATCH_VER_MOD_2 EQUAL 0 AND NOT VERSION_TWEAK_LABEL STREQUAL "local")
  set(IS_STABLE_BUILD_DEFAULT ON)
else ()
  set(IS_STABLE_BUILD_DEFAULT OFF)
endif ()
option(IS_STABLE_BUILD "Whether or not we're building a stable release" ${IS_STABLE_BUILD_DEFAULT})
set(VERSION_SEMVER "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}+${VERSION_TWEAK_LABEL}.${PROJECT_VERSION_TWEAK}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(output-paths)
include(smol-binaries)
message(STATUS "${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}")

# Handy for CI
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${CMAKE_PROJECT_VERSION}")

if (MSVC AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CLANG_CL 1 CACHE BOOL "Whether we're using clang-cl")
else ()
  set(CLANG_CL 0)
endif ()

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Simulated compiler: ${CMAKE_CXX_SIMULATE_ID}")
message(STATUS "MSVC: ${MSVC}")
message(STATUS "CLANG_CL: ${CLANG_CL}")

set(COMMON_COMPILE_OPTIONS "/DUNICODE" "/D_UNICODE")

if (MSVC AND NOT CLANG_CL)
  list(
    APPEND COMMON_COMPILE_OPTIONS

    # Standards-compliant C++20 coroutines
    "/await:strict"

    # Synchronous writes to PDB files in case building with parallel CL.exe
    "/FS"

    # Include content and marker in error messages
    "/diagnostics:caret"
  )
endif ()

add_compile_options(${COMMON_COMPILE_OPTIONS})

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Default)

add_subdirectory("third-party")
add_subdirectory("src")
add_subdirectory("reg")
add_subdirectory("scripts")
add_subdirectory("HTCC-Installer")
