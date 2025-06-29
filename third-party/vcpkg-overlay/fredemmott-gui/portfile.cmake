vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO fredemmott/fui
  REF 70b3899629e9914dab3ae4768077ccb9c1f20d9f
  SHA512 061cf2802d8df566f5a6b9273d2e72d751f6e6747785d8df939900f862f715459c4faeda2a5653b6227d5eab49739c4ef0b3fd2d09985dad6c1de298c2ae5884
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