/*
 * MIT License
 *
 * Copyright (c) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include <directxtk/SimpleMath.h>
#include <openxr/openxr.h>

using namespace DirectX::SimpleMath;

static XrPosef operator*(const XrPosef& a, const XrPosef& b) {
  const Quaternion ao {
    a.orientation.x,
    a.orientation.y,
    a.orientation.z,
    a.orientation.w,
  };
  const Quaternion bo {
    b.orientation.x,
    b.orientation.y,
    b.orientation.z,
    b.orientation.w,
  };
  const Vector3 ap {
    a.position.x,
    a.position.y,
    a.position.z,
  };
  const Vector3 bp {
    b.position.x,
    b.position.y,
    b.position.z,
  };

  auto o = ao * bo;
  o.Normalize();
  const auto p = Vector3::Transform(ap, bo) + bp;

  return {
    .orientation = {o.x, o.y, o.z, o.w},
    .position = {p.x, p.y, p.z},
  };
}
