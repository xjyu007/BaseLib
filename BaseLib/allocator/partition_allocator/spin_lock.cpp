// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "allocator/partition_allocator/spin_lock.h"

#include "threading/platform_thread.h"
#include "build_config.h"

#include <Windows.h>

// The YIELD_PROCESSOR macro wraps an architecture specific-instruction that
// informs the processor we're in a busy wait, so it can handle the branch more
// intelligently and e.g. reduce power to our core or give more resources to the
// other hyper-thread on this core. See the following for context:
// https://software.intel.com/en-us/articles/benefitting-power-and-performance-sleep-loops
//
// The YIELD_THREAD macro tells the OS to relinquish our quantum. This is
// basically a worst-case fallback, and if you're hitting it with any frequency
// you really should be using a proper lock (such as |base::Lock|)rather than
// these spinlocks.
#define YIELD_PROCESSOR YieldProcessor()
#define YIELD_THREAD SwitchToThread()

namespace base::subtle
{
		void SpinLock::LockSlow() {
			// The value of |kYieldProcessorTries| is cargo culted from TCMalloc, Windows
			// critical section defaults, and various other recommendations.
			// TODO(jschuh): Further tuning may be warranted.
			static const auto kYieldProcessorTries = 1000;
			// The value of |kYieldThreadTries| is completely made up.
			static const auto kYieldThreadTries = 10;
			auto yield_thread_count = 0;
			do {
				do {
					for (auto count = 0; count < kYieldProcessorTries; ++count) {
						// Let the processor know we're spinning.
						YIELD_PROCESSOR;
						if (!lock_.load(std::memory_order_relaxed) &&
							LIKELY(!lock_.exchange(true, std::memory_order_acquire)))
							return;
					}

					if (yield_thread_count < kYieldThreadTries) {
						++yield_thread_count;
						// Give the OS a chance to schedule something on this core.
						YIELD_THREAD;
					}
					else {
						// At this point, it's likely that the lock is held by a lower priority
						// thread that is unavailable to finish its work because of higher
						// priority threads spinning here. Sleeping should ensure that they make
						// progress.
						PlatformThread::Sleep(TimeDelta::FromMilliseconds(1));
					}
				} while (lock_.load(std::memory_order_relaxed));
			} while (UNLIKELY(lock_.exchange(true, std::memory_order_acquire)));
		}
} // namespace base
