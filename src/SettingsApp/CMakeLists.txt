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
  DemoPage.xaml.cpp DemoPage.xaml.h DemoPage.xaml DemoPage.idl
  MainWindow.xaml.cpp MainWindow.xaml.h MainWindow.xaml MainWindow.idl
)
set_property(
  SOURCE App.xaml
  PROPERTY VS_XAML_TYPE
  "ApplicationDefinition"
)
target_include_directories(
  HTCCSettings
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(HTCCSettings PRIVATE HTCCLibCommon)
target_precompile_headers(
  HTCCSettings
  PRIVATE
  pch.h
)
target_compile_options(
  HTCCSettings
  PRIVATE
  "/FI${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/HTCCSettings.dir/$<CONFIG>/cmake_pch.hxx"
)
sign_target(HTCCSettings)

include(winui3-boilerplate.cmake)
