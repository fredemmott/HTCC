# Patch up the generated vcxproj
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.props"
  "${CMAKE_CURRENT_BINARY_DIR}/Directory.Build.props"
  COPYONLY
)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.targets"
  "${CMAKE_CURRENT_BINARY_DIR}/Directory.Build.targets"
  COPYONLY
)

add_executable(
  DemoApp
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
set_property(
  TARGET DemoApp
  PROPERTY VS_PACKAGE_REFERENCES
  "Microsoft.Windows.CppWinRT_2.0.220608.4"
  "Microsoft.WindowsAppSDK_1.1.5"
  "Microsoft.Windows.SDK.BuildTools_10.0.22621.1"
  "Microsoft.Windows.ImplementationLibrary_1.0.220201.1"
)
target_include_directories(
  DemoApp
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_target_properties(
  DemoApp
  PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  # ----- C++/WinRT, Windows App SDK, and WinUI stuff starts here -----
  VS_GLOBAL_RootNamespace DemoApp
  VS_GLOBAL_AppContainerApplication false
  VS_GLOBAL_AppxPackage false
  VS_GLOBAL_CppWinRTOptimized true
  VS_GLOBAL_CppWinRTRootNamespaceAutoMerge true
  VS_GLOBAL_UseWinUI true
  VS_GLOBAL_ApplicationType "Windows Store"
  VS_GLOBAL_WindowsPackageType None
  VS_GLOBAL_EnablePreviewMsixTooling true
  VS_GLOBAL_WindowsAppSDKSelfContained true
)
