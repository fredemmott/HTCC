set(
  CMAKE_CXX_STANDARD_LIBRARIES

  # Keep stuff that NuGet added
  "%(AdditionalDependencies)"
)

# Patch up the generated vcxproj
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.props"
  "${CMAKE_BINARY_DIR}/Directory.Build.props"
  COPYONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.targets"
  "${CMAKE_CURRENT_BINARY_DIR}/Directory.Build.targets"
  COPYONLY
)

add_executable(
  HTCCSettings
  WIN32
  app.manifest
  App.xaml.cpp App.xaml.h App.xaml App.idl
  MainWindow.xaml.cpp MainWindow.xaml.h MainWindow.xaml MainWindow.idl
)
set_property(
  SOURCE App.xaml
  PROPERTY VS_XAML_TYPE
  "ApplicationDefinition"
)
target_compile_definitions(
  HTCCSettings
  PRIVATE

  # We override wwinmain to self-elevate, as the windows app SDK currently
  # is incompatible with manifest-based elevation
  DISABLE_XAML_GENERATED_MAIN=1
)
target_include_directories(
  HTCCSettings
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  HTCCSettings
  PRIVATE
  HTCCLibCommon
  System::Shell32
  System::User32
)
target_precompile_headers(
  HTCCSettings
  PRIVATE
  pch.h
)
target_compile_options(
  HTCCSettings
  PRIVATE
  "/FI${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/HTCCSettings.dir/$<CONFIG>/cmake_pch.hxx"

  # We intentionally throw from noexcept functions to get more useful
  # backtraces/crash dumps from winrt event handlers
  "/wd4297"
)
sign_target(HTCCSettings)

include(winui3-boilerplate.cmake)

set(VERSION_CPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")
add_custom_command(
  OUTPUT "${VERSION_CPP_FILE}"
  COMMAND
  "${CMAKE_COMMAND}"
  "-DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in"
  "-DOUTPUT_FILE=${VERSION_CPP_FILE}"
  "-DVERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}"
  "-DVERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}"
  "-DVERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}"
  "-DVERSION_BUILD=${CMAKE_PROJECT_VERSION_TWEAK}"
  "-DACTIVE_BUILD_MODE=$<CONFIG>"
  "-DPRODUCT_NAME=${CMAKE_PROJECT_NAME}"
  "-P${CMAKE_SOURCE_DIR}/src/version.cmake"
  DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in"
  "${CMAKE_SOURCE_DIR}/src/version.cmake"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_sources(HTCCSettings PRIVATE "${VERSION_CPP_FILE}")

install(
  DIRECTORY

  # Trailing / copies the contents instead of the directory itself,
  # i.e. avoids installing `bin/Debug/`
  "$<TARGET_FILE_DIR:HTCCSettings>/"
  DESTINATION "SettingsApp"
  PATTERN "*.pdb" EXCLUDE
  PATTERN "*.lib" EXCLUDE
)
install(
  FILES
  "$<TARGET_PDB_FILE:HTCCSettings>"
  DESTINATION "."
  COMPONENT DebugSymbols
)
