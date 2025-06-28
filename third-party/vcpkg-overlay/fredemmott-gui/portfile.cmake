vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO fredemmott/fui
  REF 7c9454d1f1089582db05c78ccb948f2773b5a3b6
  SHA512 f44469ecf5a8236b75a090b2a8859b0315d41f2efceff2344d9bbf3cce8ce217db9eb600a2e8519239bfdda8b6a6aa5066a49934c2f0038ce728272b88d168aa
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