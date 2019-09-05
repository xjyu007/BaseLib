// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "threading/scoped_thread_priority.h"

#include "containers/flat_set.h"
#include "location.h"
#include "no_destructor.h"
#include "synchronization/lock.h"
#include "threading/platform_thread.h"
#include "trace_event/trace_event.h"

namespace base {

	namespace {

		// Flags the location |from_here| as executed. Returns true if the location
		// was not previously executed.
		bool ShouldBoostThreadPriorityForLocation(const Location& from_here) {
			using ExecutedProgramCounterSet = flat_set<const void*>;
			static NoDestructor<Lock> lock;
			static NoDestructor<ExecutedProgramCounterSet> cache;

			AutoLock auto_lock(*lock);
			return cache.get()->insert(from_here.program_counter()).second;
		}

	}  // namespace

	// Enable the boost of thread priority when the code may load a library. The
	// thread priority boost is required to avoid priority inversion on the loader
	// lock.
	const Feature kBoostThreadPriorityOnLibraryLoading{
	    "BoostThreadPriorityOnLibraryLoading", FEATURE_DISABLED_BY_DEFAULT};

	ScopedThreadMayLoadLibraryOnBackgroundThread::
		ScopedThreadMayLoadLibraryOnBackgroundThread(const Location& from_here) {
		//TRACE_EVENT_BEGIN2("base", "ScopedThreadMayLoadLibraryOnBackgroundThread",
		//					 "file_name", from_here.file_name(), "function_name", 
		//					 from_here.function_name());

		if (!FeatureList::IsEnabled(kBoostThreadPriorityOnLibraryLoading))
			return;

		// If the code at |from_here| has already been executed, do not boost the
		// thread priority.
		if (!ShouldBoostThreadPriorityForLocation(from_here))
			return;

		auto priority = PlatformThread::GetCurrentThreadPriority();
		if (priority == ThreadPriority::BACKGROUND) {
			original_thread_priority_ = priority;
			PlatformThread::SetCurrentThreadPriority(ThreadPriority::NORMAL);
		}
	}

	ScopedThreadMayLoadLibraryOnBackgroundThread::
		~ScopedThreadMayLoadLibraryOnBackgroundThread() {
		//TRACE_EVENT_END0("base", "ScopedThreadMayLoadLibraryOnBackgroundThread");
		if (original_thread_priority_)
			PlatformThread::SetCurrentThreadPriority(original_thread_priority_.value());
	}

}  // namespace base
