// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"

namespace base {
	typedef void (*PartitionAllocOomCallback)();
	// Registers a callback to be invoked during an OOM_CRASH(). OOM_CRASH is
	// invoked by users of PageAllocator (including PartitionAlloc) to signify an
	// allocation failure from the platform.
	BASE_EXPORT void SetPartitionAllocOomCallback(
	    PartitionAllocOomCallback callback);

	namespace internal {
		BASE_EXPORT void RunPartitionAllocOomCallback();
	}  // namespace internal

}  // namespace base
