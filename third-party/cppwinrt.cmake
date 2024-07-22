include(ExternalProject)

set(CPPWINRT_VERSION "2.0.240405.15")

ExternalProject_Add(
  CppWinRTNuget
  URL "https://www.nuget.org/api/v2/package/Microsoft.Windows.CppWinRT/${CPPWINRT_VERSION}"
  URL_HASH "SHA256=e889007b5d9235931e7340ddf737d2c346eebdd23c619f1f4f2426a2aae47180"

  CONFIGURE_COMMAND ""
  BUILD_COMMAND
  "<SOURCE_DIR>/bin/cppwinrt.exe"
  -in "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
  -output "<BINARY_DIR>/include"
  INSTALL_COMMAND ""

  EXCLUDE_FROM_ALL
)

ExternalProject_Get_property(CppWinRTNuget SOURCE_DIR)
ExternalProject_Get_property(CppWinRTNuget BINARY_DIR)

add_library(CppWinRT INTERFACE)
add_dependencies(CppWinRT CppWinRTNuget)
target_include_directories(CppWinRT INTERFACE "${BINARY_DIR}/include")
target_link_libraries(CppWinRT INTERFACE System::WindowsApp)
add_library(ThirdParty::CppWinRT ALIAS CppWinRT)

install(
  FILES
  "${SOURCE_DIR}/LICENSE"
  TYPE DOC
  RENAME "LICENSE-ThirdParty-CppWinRT.txt"
)
