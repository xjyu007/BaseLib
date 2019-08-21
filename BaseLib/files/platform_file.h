#pragma once

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win/scoped_handle.h"

// This file defines platform-independent types for dealing with
// platform-dependent files. If possible, use the higher-level base::File class
// rather than these primitives.

namespace base {

	using PlatformFile = HANDLE;
	using ScopedPlatformFile = ::base::win::ScopedHandle;

	// It would be nice to make this constexpr but INVALID_HANDLE_VALUE is a
	// ((void*)(-1)) which Clang rejects since reinterpret_cast is technically
	// disallowed in constexpr. Visual Studio accepts this, however.
	const PlatformFile kInvalidPlatformFile = INVALID_HANDLE_VALUE;

}  // namespace
