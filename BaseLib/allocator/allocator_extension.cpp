// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "allocator/allocator_extension.h"
#include "logging.h"

/*#if BUILDFLAG(USE_TCMALLOC)
#include "third_party/tcmalloc/chromium/src/gperftools/heap-profiler.h"
#include "third_party/tcmalloc/chromium/src/gperftools/malloc_extension.h"
#include "third_party/tcmalloc/chromium/src/gperftools/malloc_hook.h"
#endif*/


namespace base::allocator {

	void ReleaseFreeMemory() {
#if defined(USE_TCMALLOC)
		//::MallocExtension::instance()->ReleaseFreeMemory();
#endif
	}

	bool GetNumericProperty(const char* name, size_t* value) {
#if defined(USE_TCMALLOC)
		//return ::MallocExtension::instance()->GetNumericProperty(name, value);
#endif
		return false;
	}

	bool SetNumericProperty(const char* name, size_t value) {
#if defined(USE_TCMALLOC)
		//return ::MallocExtension::instance()->SetNumericProperty(name, value);
#endif
		return false;
	}

	void GetHeapSample(std::string* writer) {
#if defined(USE_TCMALLOC)
		//::MallocExtension::instance()->GetHeapSample(writer);
#endif
	}

	bool IsHeapProfilerRunning() {
#if defined(USE_TCMALLOC) && defined(ENABLE_PROFILING)
		//return ::IsHeapProfilerRunning();
#endif
		return false;
	}

	void SetHooks(AllocHookFunc alloc_hook, FreeHookFunc free_hook) {
		// TODO(sque): Use allocator shim layer instead.
#if defined(USE_TCMALLOC)
// Make sure no hooks get overwritten.
		/*const auto prev_alloc_hook = MallocHook::SetNewHook(alloc_hook);
		if (alloc_hook)
			DCHECK(!prev_alloc_hook);

		const auto prev_free_hook = MallocHook::SetDeleteHook(free_hook);
		if (free_hook)
			DCHECK(!prev_free_hook);*/
#endif
	}

	int GetCallStack(void** stack, int max_stack_size) {
#if defined(USE_TCMALLOC)
		//return MallocHook::GetCallerStackTrace(stack, max_stack_size, 0);
#endif
		return 0;
	}
} // namespace base
