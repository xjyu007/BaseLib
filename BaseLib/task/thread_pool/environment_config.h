// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>

#include "base_export.h"
#include "task/task_traits.h"
#include "threading/thread.h"

namespace base {
	namespace internal {

		// TODO(etiennep): This is now specific to
		// PooledSingleThreadTaskRunnerManager, move it there.
		enum EnvironmentType {
			FOREGROUND = 0,
			FOREGROUND_BLOCKING,
			BACKGROUND,
			BACKGROUND_BLOCKING,
			ENVIRONMENT_COUNT  // Always last.
		};

		// Order must match the EnvironmentType enum.
		struct EnvironmentParams {
			// The threads and histograms of this environment will be labeled with
			// the thread pool name concatenated to this.
			const char* name_suffix;

			// Preferred priority for threads in this environment; the actual thread
			// priority depends on shutdown state and platform capabilities.
			ThreadPriority priority_hint;
		};

		constexpr EnvironmentParams kEnvironmentParams[] = {
			{ "Foreground", base::ThreadPriority::NORMAL },
		{ "ForegroundBlocking", base::ThreadPriority::NORMAL },
		{ "Background", base::ThreadPriority::BACKGROUND },
		{ "BackgroundBlocking", base::ThreadPriority::BACKGROUND },
		};

		// Returns true if this platform supports having SchedulerWorkers running with a
		// background priority.
		bool BASE_EXPORT CanUseBackgroundPriorityForWorkerThread();

	}  // namespace internal
}  // namespace base
