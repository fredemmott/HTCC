vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO fredemmott/fui
  REF e70f6a8d23e2cc17287a4438f0e8caec59816074
  SHA512 d3bba7b3938e10fa4c280e446d170f5cd764431698ca37a83ac4e70cd453b17382a1fc90fa0867403f55db40ce3989652b424101d7439e61966c53f03dec5d2a
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
# bump