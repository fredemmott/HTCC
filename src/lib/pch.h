// Copyright (c) 2022-present Frederick Emmott
// SPDX-License-Identifier: MIT
#pragma once

#define DIRECTINPUT_VERSION 0x0800
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define XR_USE_PLATFORM_WIN32

// Include order matters for these, so don't let clang-format reorder them
// clang-format off
#include <Windows.h>
#include <Unknwn.h>
#include <winrt/base.h>
// clang-format on

#include <format>

#include "openxr.h"

template <class CharT>
struct std::formatter<XrResult, CharT> : std::formatter<int, CharT> {};
