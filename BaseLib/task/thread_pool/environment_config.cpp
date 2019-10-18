// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "task/thread_pool/environment_config.h"

#include "synchronization/lock.h"
#include "threading/platform_thread.h"

namespace base::internal {

	namespace {

		bool CanUseBackgroundPriorityForWorkerThreadImpl() {
			// When Lock doesn't handle multiple thread priorities, run all
			// WorkerThread with a normal priority to avoid priority inversion when a
			// thread running with a normal priority tries to acquire a lock held by a
			// thread running with a background priority.
			if (!Lock::HandlesMultipleThreadPriorities())
				return false;

			// When thread priority can't be increased to NORMAL, run all threads with a
			// NORMAL priority to avoid priority inversions on shutdown (ThreadPoolImpl
			// increases BACKGROUND threads priority to NORMAL on shutdown while resolving
			// remaining shutdown blocking tasks).

			return true;
		}

	}  // namespace

	bool CanUseBackgroundPriorityForWorkerThread() {
		static const bool can_use_background_priority_for_worker_thread =
			CanUseBackgroundPriorityForWorkerThreadImpl();
		return can_use_background_priority_for_worker_thread;
	}
} // namespace base
