// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "macros.h"
#include "memory/singleton.h"
#include "synchronization/lock.h"
#include "trace_event/memory_dump_provider.h"

#define MALLOC_MEMORY_TRACING_SUPPORTED

namespace base {
	namespace trace_event {

		// Dump provider which collects process-wide memory stats.
		class BASE_EXPORT MallocDumpProvider : public MemoryDumpProvider {
		public:
			// Name of the allocated_objects dump. Use this to declare suballocator dumps
			// from other dump providers.
			static const char kAllocatedObjects[];

			static MallocDumpProvider* GetInstance();

			// MemoryDumpProvider implementation.
			bool OnMemoryDump(const MemoryDumpArgs& args,
				ProcessMemoryDump* pmd) override;

			// Used by out-of-process heap-profiling. When malloc is profiled by an
			// external process, that process will be responsible for emitting metrics on
			// behalf of this one. Thus, MallocDumpProvider should not do anything.
			void EnableMetrics();
			void DisableMetrics();

		private:
			friend struct DefaultSingletonTraits<MallocDumpProvider>;

			MallocDumpProvider();
			~MallocDumpProvider() override;

			bool emit_metrics_on_memory_dump_ = true;
			base::Lock emit_metrics_on_memory_dump_lock_;

			DISALLOW_COPY_AND_ASSIGN(MallocDumpProvider);
		};

	}  // namespace trace_event
}  // namespace base
