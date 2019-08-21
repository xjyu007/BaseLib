// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "synchronization/atomic_flag.h"

#include "logging.h"

#if defined(OS_WIN)
#include "win/windows_types.h"
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#include <errno.h>
#include <pthread.h>
#endif

namespace base {

	AtomicFlag::AtomicFlag() {
		// It doesn't matter where the AtomicFlag is built so long as it's always
		// Set() from the same sequence after. Note: the sequencing requirements are
		// necessary for IsSet()'s callers to know which sequence's memory operations
		// they are synchronized with.
		DETACH_FROM_SEQUENCE(set_sequence_checker_);
	}

	AtomicFlag::~AtomicFlag() = default;

	void AtomicFlag::Set() {
		DCHECK_CALLED_ON_VALID_SEQUENCE(set_sequence_checker_);
		flag_.store(1, std::memory_order_release);
	}

	void AtomicFlag::UnsafeResetForTesting() {
		flag_.store(0, std::memory_order_release);
	}

} // namespace base