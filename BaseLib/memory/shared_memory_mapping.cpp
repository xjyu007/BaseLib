// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "memory/shared_memory_mapping.h"
#include "logging.h"
#include "memory/shared_memory_tracker.h"
#include "unguessable_token.h"
#include "build_config.h"

#include <AclAPI.h>

namespace base {

	SharedMemoryMapping::SharedMemoryMapping() = default;

	SharedMemoryMapping::SharedMemoryMapping(SharedMemoryMapping&& mapping) noexcept
		: memory_(mapping.memory_),
		size_(mapping.size_),
		mapped_size_(mapping.mapped_size_),
		guid_(mapping.guid_) {
		mapping.memory_ = nullptr;
	}

	SharedMemoryMapping& SharedMemoryMapping::operator=(SharedMemoryMapping&& mapping)  noexcept {
		Unmap();
		memory_ = mapping.memory_;
		size_ = mapping.size_;
		mapped_size_ = mapping.mapped_size_;
		guid_ = mapping.guid_;
		mapping.memory_ = nullptr;
		return *this;
	}

	SharedMemoryMapping::~SharedMemoryMapping() {
		Unmap();
	}

	SharedMemoryMapping::SharedMemoryMapping(void* memory, size_t size, size_t mapped_size, const UnguessableToken& guid)
		: memory_(memory), size_(size), mapped_size_(mapped_size), guid_(guid) {
		SharedMemoryTracker::GetInstance()->IncrementMemoryUsage(*this);
	}

	void SharedMemoryMapping::Unmap() const {
		if (!IsValid())
			return;

		SharedMemoryTracker::GetInstance()->DecrementMemoryUsage(*this);
		if (!UnmapViewOfFile(memory_))
			DPLOG(ERROR) << "UnmapViewOfFile";
	}

	ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping() = default;
	ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(ReadOnlySharedMemoryMapping&&) noexcept = default;
	ReadOnlySharedMemoryMapping& ReadOnlySharedMemoryMapping::operator=(ReadOnlySharedMemoryMapping&&) noexcept = default;
	ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(void* address, size_t size, size_t mapped_size, const UnguessableToken& guid)
		: SharedMemoryMapping(address, size, mapped_size, guid) {
	}

	WritableSharedMemoryMapping::WritableSharedMemoryMapping() = default;
	WritableSharedMemoryMapping::WritableSharedMemoryMapping(WritableSharedMemoryMapping&&) noexcept = default;
	WritableSharedMemoryMapping& WritableSharedMemoryMapping::operator=(WritableSharedMemoryMapping&&) noexcept = default;
	WritableSharedMemoryMapping::WritableSharedMemoryMapping(void* address, size_t size, size_t mapped_size, const UnguessableToken& guid)
		: SharedMemoryMapping(address, size, mapped_size, guid) {
	}

} // namespace base