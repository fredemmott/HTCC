vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO fredemmott/fui
  REF 470c81d14f6be9552c4f503c1fdf7df987b0a750
  SHA512 041aafd2fd868cf266931b6628f7bc9992ff620ee676fe54597ae5ec4285c50b1674adc7bf59f7f3f67ccefe5b13bf17a33390ac04baf3d893853edb668ef831
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