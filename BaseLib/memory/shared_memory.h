#pragma once

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base_export.h"
#include "macros.h"
#include "memory/shared_memory_handle.h"
#include "build_config.h"

namespace base {

	class FilePath;

	// Options for creating a shared memory object.
	struct BASE_EXPORT SharedMemoryCreateOptions {
		// Size of the shared memory object to be created.
		// When opening an existing object, this has no effect.
		size_t size = 0;

		// If true, mappings might need to be made executable later.
		bool executable = false;

		// If true, the file can be shared read-only to a process.
		bool share_read_only = false;
	};

	// Platform abstraction for shared memory.
	// SharedMemory consumes a SharedMemoryHandle [potentially one that it created]
	// to map a shared memory OS resource into the virtual address space of the
	// current process.
	//
	// DEPRECATED - Use {Writable,ReadOnly}SharedMemoryRegion instead.
	// http://crbug.com/795291
	class BASE_EXPORT SharedMemory {
	public:
		SharedMemory();

		// Similar to the default constructor, except that this allows for
		// calling LockDeprecated() to acquire the named mutex before either Create or
		// Open are called on Windows.
		explicit SharedMemory(const std::wstring& name);

		// Create a new SharedMemory object from an existing, open
		// shared memory file.
		//
		// WARNING: This does not reduce the OS-level permissions on the handle; it
		// only affects how the SharedMemory will be mmapped. Use
		// GetReadOnlyHandle to drop permissions. TODO(jln,jyasskin): DCHECK
		// that |read_only| matches the permissions of the handle.
		SharedMemory(const SharedMemoryHandle& handle, bool read_only);

		// Closes any open files.
		~SharedMemory();

		// Return true iff the given handle is valid (i.e. not the distingished
		// invalid value; NULL for a HANDLE and -1 for a file descriptor)
		static bool IsHandleValid(const SharedMemoryHandle& handle);

		// Closes a shared memory handle.
		static void CloseHandle(const SharedMemoryHandle& handle);

		// Duplicates The underlying OS primitive. Returns an invalid handle on
		// failure. The caller is responsible for destroying the duplicated OS
		// primitive.
		static SharedMemoryHandle DuplicateHandle(const SharedMemoryHandle& handle);

		// Creates a shared memory object as described by the options struct.
		// Returns true on success and false on failure.
		bool Create(const SharedMemoryCreateOptions& options);

		// Creates and maps an anonymous shared memory segment of size size.
		// Returns true on success and false on failure.
		bool CreateAndMapAnonymous(size_t size);

		// Creates an anonymous shared memory segment of size size.
		// Returns true on success and false on failure.
		bool CreateAnonymous(size_t size) {
			SharedMemoryCreateOptions options;
			options.size = size;
			return Create(options);
		}

		// Maps the shared memory into the caller's address space.
		// Returns true on success, false otherwise.  The memory address
		// is accessed via the memory() accessor.  The mapped address is guaranteed to
		// have an alignment of at least MAP_MINIMUM_ALIGNMENT. This method will fail
		// if this object is currently mapped.
		bool Map(size_t bytes) {
			return MapAt(0, bytes);
		}

		// Same as above, but with |offset| to specify from begining of the shared
		// memory block to map.
		// |offset| must be alignent to value of |SysInfo::VMAllocationGranularity()|.
		bool MapAt(off_t offset, size_t bytes);
		enum { MAP_MINIMUM_ALIGNMENT = 32 };

		// Unmaps the shared memory from the caller's address space.
		// Returns true if successful; returns false on error or if the
		// memory is not mapped.
		bool Unmap();

		// The size requested when the map is first created.
		[[nodiscard]] size_t requested_size() const { return requested_size_; }

		// The actual size of the mapped memory (may be larger than requested).
		[[nodiscard]] size_t mapped_size() const { return mapped_size_; }

		// Gets a pointer to the opened memory space if it has been
		// Mapped via Map().  Returns NULL if it is not mapped.
		[[nodiscard]] void* memory() const { return memory_; }

		// Returns the underlying OS handle for this segment.
		// Use of this handle for anything other than an opaque
		// identifier is not portable.
		[[nodiscard]] SharedMemoryHandle handle() const;

		// Returns the underlying OS handle for this segment. The caller takes
		// ownership of the handle and memory is unmapped. This is equivalent to
		// duplicating the handle and then calling Unmap() and Close() on this object,
		// without the overhead of duplicating the handle.
		SharedMemoryHandle TakeHandle();

		// Closes the open shared memory segment. The memory will remain mapped if
		// it was previously mapped.
		// It is safe to call Close repeatedly.
		void Close();

		// Returns a read-only handle to this shared memory region. The caller takes
		// ownership of the handle. For POSIX handles, CHECK-fails if the region
		// wasn't Created or Opened with share_read_only=true, which is required to
		// make the handle read-only. When the handle is passed to the IPC subsystem,
		// that takes ownership of the handle. As such, it's not valid to pass the
		// sample handle to the IPC subsystem twice. Returns an invalid handle on
		// failure.
		[[nodiscard]] SharedMemoryHandle GetReadOnlyHandle() const;

		// Returns an ID for the mapped region. This is ID of the SharedMemoryHandle
		// that was mapped. The ID is valid even after the SharedMemoryHandle is
		// Closed, as long as the region is not unmapped.
		[[nodiscard]] const UnguessableToken& mapped_id() const { return mapped_id_; }

	private:
		// If true indicates this came from an external source so needs extra checks
		// before being mapped.
		bool external_section_ = false;

		// The OS primitive that backs the shared memory region.
		SharedMemoryHandle shm_;

		size_t mapped_size_ = 0;
		void* memory_ = nullptr;
		bool read_only_ = false;
		size_t requested_size_ = 0;
		base::UnguessableToken mapped_id_;

		DISALLOW_COPY_AND_ASSIGN(SharedMemory);
	};

} // namespace base
