// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "trace_event/malloc_dump_provider.h"

#include "allocator/allocator_extension.h"
#include "trace_event/process_memory_dump.h"
#include "trace_event/traced_value.h"
#include "build_config.h"

#include <malloc.h>
#include <Windows.h>

namespace base::trace_event {

	namespace {
		// A structure containing some information about a given heap.
		struct WinHeapInfo {
			size_t committed_size;
			size_t uncommitted_size;
			size_t allocated_size;
			size_t block_count;
		};

		// NOTE: crbug.com/665516
		// Unfortunately, there is no safe way to collect information from secondary
		// heaps due to limitations and racy nature of this piece of WinAPI.
		void WinHeapMemoryDumpImpl(WinHeapInfo* crt_heap_info) {
			// Iterate through whichever heap our CRT is using.
			const auto crt_heap = reinterpret_cast<HANDLE>(_get_heap_handle());
			::HeapLock(crt_heap);
			PROCESS_HEAP_ENTRY heap_entry;
			heap_entry.lpData = nullptr;
			// Walk over all the entries in the main heap.
			while (::HeapWalk(crt_heap, &heap_entry) != FALSE) {
				if ((heap_entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) {
					crt_heap_info->allocated_size += heap_entry.cbData;
					crt_heap_info->block_count++;
				} else if ((heap_entry.wFlags & PROCESS_HEAP_REGION) != 0) {
					crt_heap_info->committed_size += heap_entry.Region.dwCommittedSize;
					crt_heap_info->uncommitted_size += heap_entry.Region.dwUnCommittedSize;
				}
			}
			CHECK(::HeapUnlock(crt_heap) == TRUE);
		}
	}  // namespace

	// static
	const char MallocDumpProvider::kAllocatedObjects[] = "malloc/allocated_objects";

	// static
	MallocDumpProvider* MallocDumpProvider::GetInstance() {
		return Singleton<MallocDumpProvider,
			LeakySingletonTraits<MallocDumpProvider>>::get();
	}

	MallocDumpProvider::MallocDumpProvider() = default;
	MallocDumpProvider::~MallocDumpProvider() = default;

	// Called at trace dump point time. Creates a snapshot the memory counters for
	// the current process.
	bool MallocDumpProvider::OnMemoryDump(const MemoryDumpArgs& args,
		ProcessMemoryDump* pmd) {
			{
				base::AutoLock auto_lock(emit_metrics_on_memory_dump_lock_);
				if (!emit_metrics_on_memory_dump_)
					return true;
			}

			size_t total_virtual_size = 0;
			size_t resident_size = 0;
			size_t allocated_objects_size = 0;
			size_t allocated_objects_count = 0;
#if defined(USE_TCMALLOC)
			auto res =
				allocator::GetNumericProperty("generic.heap_size", &total_virtual_size);
			DCHECK(res);
			res = allocator::GetNumericProperty("generic.total_physical_bytes",
				&resident_size);
			DCHECK(res);
			res = allocator::GetNumericProperty("generic.current_allocated_bytes",
				&allocated_objects_size);
			DCHECK(res);
#else
			// This is too expensive on Windows, crbug.com/780735.
			if (args.level_of_detail == MemoryDumpLevelOfDetail::DETAILED) {
				WinHeapInfo main_heap_info = {};
				WinHeapMemoryDumpImpl(&main_heap_info);
				total_virtual_size =
					main_heap_info.committed_size + main_heap_info.uncommitted_size;
				// Resident size is approximated with committed heap size. Note that it is
				// possible to do this with better accuracy on windows by intersecting the
				// working set with the virtual memory ranges occuipied by the heap. It's
				// not clear that this is worth it, as it's fairly expensive to do.
				resident_size = main_heap_info.committed_size;
				allocated_objects_size = main_heap_info.allocated_size;
				allocated_objects_count = main_heap_info.block_count;
			}
#endif

			auto outer_dump = pmd->CreateAllocatorDump("malloc");
			outer_dump->AddScalar("virtual_size", MemoryAllocatorDump::kUnitsBytes,
				total_virtual_size);
			outer_dump->AddScalar(MemoryAllocatorDump::kNameSize,
				MemoryAllocatorDump::kUnitsBytes, resident_size);

			auto inner_dump = pmd->CreateAllocatorDump(kAllocatedObjects);
			inner_dump->AddScalar(MemoryAllocatorDump::kNameSize,
				MemoryAllocatorDump::kUnitsBytes,
				allocated_objects_size);
			if (allocated_objects_count != 0) {
				inner_dump->AddScalar(MemoryAllocatorDump::kNameObjectCount,
					MemoryAllocatorDump::kUnitsObjects,
					allocated_objects_count);
			}

			if (resident_size > allocated_objects_size) {
				// Explicitly specify why is extra memory resident. In tcmalloc it accounts
				// for free lists and caches. In mac and ios it accounts for the
				// fragmentation and metadata.
				auto other_dump =
					pmd->CreateAllocatorDump("malloc/metadata_fragmentation_caches");
				other_dump->AddScalar(MemoryAllocatorDump::kNameSize,
					MemoryAllocatorDump::kUnitsBytes,
					resident_size - allocated_objects_size);
			}
			return true;
	}

	void MallocDumpProvider::EnableMetrics() {
		AutoLock auto_lock(emit_metrics_on_memory_dump_lock_);
		emit_metrics_on_memory_dump_ = true;
	}

	void MallocDumpProvider::DisableMetrics() {
		AutoLock auto_lock(emit_metrics_on_memory_dump_lock_);
		emit_metrics_on_memory_dump_ = false;
	}
} // namespace base
