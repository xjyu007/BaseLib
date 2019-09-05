#pragma once

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains internal routines that are called by other files in
// base/process/.

#include "environment.h"

namespace base {
	namespace internal {

		// Returns a modified environment vector constructed from the given environment
		// and the list of changes given in |changes|. Each key in the environment is
		// matched against the first element of the pairs. In the event of a match, the
		// value is replaced by the second of the pair, unless the second is empty, in
		// which case the key-value is removed.
		//
		// This Windows version takes and returns a Windows-style environment block,
		// which is a string containing several null-terminated strings followed by an
		// extra terminating null character. So, e.g., the environment A=1 B=2 is
		// represented as L"A=1\0B=2\0\0".
		BASE_EXPORT NativeEnvironmentString AlterEnvironment(const wchar_t* env, const EnvironmentMap& changes);

	}  // namespace internal
}  // namespace base
