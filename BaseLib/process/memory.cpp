// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "debug_/alias.h"
#include "process/memory.h"

namespace base {

	bool UncheckedCalloc(size_t num_items, size_t size, void** result) {
		const size_t alloc_size = num_items * size;

		// Overflow check
		if (size && ((alloc_size / size) != num_items)) {
			*result = nullptr;
			return false;
		}

		if (!UncheckedMalloc(alloc_size, result))
			return false;

		memset(*result, 0, alloc_size);
		return true;
	}

}  // namespace base
