// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#include <directxtk/SimpleMath.h>
#include <openxr/openxr.h>

#include <format>

static constexpr XrPosef XR_POSEF_IDENTITY {
  .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
  .position = {0.0f, 0.0f, 0.0f},
};

static inline DirectX::SimpleMath::Quaternion XrQuatToSM(
  const XrQuaternionf& quat) {
  using namespace DirectX::SimpleMath;
  static_assert(sizeof(XrQuaternionf) == sizeof(Quaternion));
  static_assert(offsetof(XrQuaternionf, w) == offsetof(Quaternion, w));
  return *reinterpret_cast<const Quaternion*>(&quat);
}

static inline XrQuaternionf SMQuatToXr(
  const DirectX::SimpleMath::Quaternion& quat) {
  return *reinterpret_cast<const XrQuaternionf*>(&quat);
}

static inline DirectX::SimpleMath::Vector3 XrVecToSM(const XrVector3f& vec) {
  using namespace DirectX::SimpleMath;
  static_assert(sizeof(XrVector3f) == sizeof(Vector3));
  static_assert(offsetof(XrVector3f, z) == offsetof(Vector3, z));
  return *reinterpret_cast<const Vector3*>(&vec);
}

static inline XrVector3f SMVecToXr(const DirectX::SimpleMath::Vector3& vec) {
  return *reinterpret_cast<const XrVector3f*>(&vec);
}

static XrPosef operator*(const XrPosef& a, const XrPosef& b) {
  using namespace DirectX::SimpleMath;
  const auto ao = XrQuatToSM(a.orientation);
  const auto bo = XrQuatToSM(b.orientation);
  const auto ap = XrVecToSM(a.position);
  const auto bp = XrVecToSM(b.position);

  auto o = ao * bo;
  const auto p = Vector3::Transform(ap, bo) + bp;

  return {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };
}

static XrQuaternionf operator*(const XrQuaternionf& a, const XrQuaternionf& b) {
  using namespace DirectX::SimpleMath;
  const auto& sma = XrQuatToSM(a);
  const auto& smb = XrQuatToSM(b);
  return SMQuatToXr(sma * smb);
}
