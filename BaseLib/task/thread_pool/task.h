#pragma once

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base_export.h"
#include "callback.h"
#include "location.h"
#include "macros.h"
#include "pending_task.h"
#include "time/time.h"

namespace base {
	namespace internal {

		// A task is a unit of work inside the thread pool. Support for tracing and
		// profiling inherited from PendingTask.
		// TODO(etiennep): This class is now equivalent to PendingTask, remove it.
		struct BASE_EXPORT Task : PendingTask {
			Task();

			// |posted_from| is the site the task was posted from. |task| is the closure
			// to run. |delay| is a delay that must expire before the Task runs.
			Task(const Location& posted_from, OnceClosure task, TimeDelta delay);

			// Task is move-only to avoid mistakes that cause reference counts to be
			// accidentally bumped.
			Task(Task&& other) noexcept;

			~Task();

			Task& operator=(Task&& other);

		private:
			DISALLOW_COPY_AND_ASSIGN(Task);
		};

	}  // namespace internal
}  // namespace base
