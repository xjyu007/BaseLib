#pragma once

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base_export.h"
#include "macros.h"
#include "task/task_traits.h"

namespace base {
	namespace internal {

		class BASE_EXPORT ScopedSetTaskPriorityForCurrentThread {
		public:
			// Within the scope of this object, GetTaskPriorityForCurrentThread() will
			// return |priority|.
			ScopedSetTaskPriorityForCurrentThread(TaskPriority priority);
			~ScopedSetTaskPriorityForCurrentThread();

		private:
			const TaskPriority priority_;

			DISALLOW_COPY_AND_ASSIGN(ScopedSetTaskPriorityForCurrentThread);
		};

		// Returns the priority of the ThreadPool task running on the current thread,
		// or TaskPriority::USER_VISIBLE if no ThreadPool task is running on the
		// current thread.
		BASE_EXPORT TaskPriority GetTaskPriorityForCurrentThread();

	}  // namespace internal
}  // namespace base

