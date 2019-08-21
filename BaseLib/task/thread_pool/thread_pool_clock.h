// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "macros.h"
#include "time/time.h"

namespace base {

	class TickClock;

	namespace internal {

		class BASE_EXPORT ThreadPoolClock {
		public:
			// |tick_clock| will service ThreadPoolClock::Now() in the scope of this
			// ThreadPoolClock.
			ThreadPoolClock(const TickClock* tick_clock);
			~ThreadPoolClock();

			// Returns the current TimeTicks::Now(). All calls to TimeTicks::Now() in
			// base/task/thread_pool should use this or
			// subtle::TimeTicksNowIgnoringOverride() depending on whether they want to
			// respect mock time (e.g. delayed tasks) or need real-time timeouts (e.g.
			// recycling threads). TODO(gab): Make MOCK_TIME always mock TimeTicks::Now()
			// and get rid of this indirection.
			static TimeTicks Now();

		private:
			DISALLOW_COPY_AND_ASSIGN(ThreadPoolClock);
		};

	}  // namespace internal
}  // namespace base
