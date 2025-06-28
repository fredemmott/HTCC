vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO fredemmott/fui
  REF a4c7fd43a384346b29548fafc2494a205f29bef9
  SHA512 c8f9f5b47e13df1692c5b4466a84132a761b800be308dbbe7c289cc8ef314e8f83a81a6d03b22cbab61dec16c1e431bff78a7cdb02e742adddc364ae03a3a8a7
  HEAD_REF main
)

vcpkg_check_features(
  OUT_FEATURE_OPTIONS FEATURE_OPTIONS
  FEATURES
  direct2d ENABLE_DIRECT2D
  skia ENABLE_SKIA
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    GENERATOR Ninja
    OPTIONS
    -DENABLE_IMPLICIT_BACKENDS=OFF
    -DENABLE_DEVELOPER_OPTIONS=OFF
    -DENABLE_DIRECT2D=${ENABLE_DIRECT2D}
    -DENABLE_SKIA=${ENABLE_SKIA}
)

vcpkg_cmake_install()

# TODO:
# Uncomment the line below if necessary to install the license file for the port
# as a file named `copyright` to the directory `${CURRENT_PACKAGES_DIR}/share/${PORT}`
# vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")