// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"

namespace base {
	namespace debug {
		namespace win {

			// Creates a synthetic heap corruption that causes the current process to
			// terminate immediately with a fast fail exception.
			[[noreturn]] BASE_EXPORT void TerminateWithHeapCorruption();

		}  // namespace win
	}  // namespace debug
}  // namespace base
