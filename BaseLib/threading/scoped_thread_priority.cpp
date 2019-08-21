// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "threading/scoped_thread_priority.h"

#include "location.h"
#include "threading/platform_thread.h"
#include "trace_event/trace_event.h"

namespace base {

#if defined(OS_WIN)
	// Enable the boost of thread priority when the code may load a library. The
	// thread priority boost is required to avoid priority inversion on the loader
	// lock.
	const Feature kBoostThreadPriorityOnLibraryLoading{
	    "BoostThreadPriorityOnLibraryLoading", FEATURE_DISABLED_BY_DEFAULT};
#endif  // OS_WIN

	ScopedThreadMayLoadLibraryOnBackgroundThread::
		ScopedThreadMayLoadLibraryOnBackgroundThread(const Location& from_here) {
		//TRACE_EVENT_BEGIN2("base", "ScopedThreadMayLoadLibraryOnBackgroundThread","file_name", from_here.file_name(), "function_name", from_here.function_name());

#if defined(OS_WIN)
		if (!FeatureList::IsEnabled(kBoostThreadPriorityOnLibraryLoading))
			return;

		ThreadPriority priority = PlatformThread::GetCurrentThreadPriority();
		if (priority == ThreadPriority::BACKGROUND) {
			original_thread_priority_ = priority;
			PlatformThread::SetCurrentThreadPriority(ThreadPriority::NORMAL);
		}
#endif  // OS_WIN
	}

	ScopedThreadMayLoadLibraryOnBackgroundThread::
		~ScopedThreadMayLoadLibraryOnBackgroundThread() {
		//TRACE_EVENT_END0("base", "ScopedThreadMayLoadLibraryOnBackgroundThread");
#if defined(OS_WIN)
		if (original_thread_priority_)
			PlatformThread::SetCurrentThreadPriority(original_thread_priority_.value());
#endif  // OS_WIN
	}

}  // namespace base
