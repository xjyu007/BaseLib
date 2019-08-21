#pragma once

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "unguessable_token.h"
#include "process/process_handle.h"
#include "win/windows_types.h"

namespace base {

	// SharedMemoryHandle is the smallest possible IPC-transportable "reference" to
	// a shared memory OS resource. A "reference" can be consumed exactly once [by
	// base::SharedMemory] to map the shared memory OS resource into the virtual
	// address space of the current process.
	// TODO(erikchen): This class should have strong ownership semantics to prevent
	// leaks of the underlying OS resource. https://crbug.com/640840.
	//
	// DEPRECATED - Use {Writable,ReadOnly}SharedMemoryRegion instead.
	// http://crbug.com/795291
	class BASE_EXPORT SharedMemoryHandle {
	public:
		// The default constructor returns an invalid SharedMemoryHandle.
		SharedMemoryHandle();

		// Standard copy constructor. The new instance shares the underlying OS
		// primitives.
		SharedMemoryHandle(const SharedMemoryHandle& handle);

		// Standard assignment operator. The updated instance shares the underlying
		// OS primitives.
		SharedMemoryHandle& operator=(const SharedMemoryHandle& handle);

		// Closes the underlying OS resource.
		// The fact that this method needs to be "const" is an artifact of the
		// original interface for base::SharedMemory::CloseHandle.
		// TODO(erikchen): This doesn't clear the underlying reference, which seems
		// like a bug, but is how this class has always worked. Fix this:
		// https://crbug.com/716072.
		void Close() const;

		// Whether ownership of the underlying OS resource is implicitly passed to
		// the IPC subsystem during serialization.
		void SetOwnershipPassesToIPC(bool ownership_passes);
		bool OwnershipPassesToIPC() const;

		// Whether the underlying OS resource is valid.
		bool IsValid() const;

		// Duplicates the underlying OS resource. Using the return value as a
		// parameter to an IPC message will cause the IPC subsystem to consume the OS
		// resource.
		SharedMemoryHandle Duplicate() const;

		// Uniques identifies the shared memory region that the underlying OS resource
		// points to. Multiple SharedMemoryHandles that point to the same shared
		// memory region will have the same GUID. Preserved across IPC.
		base::UnguessableToken GetGUID() const;

		// Returns the size of the memory region that SharedMemoryHandle points to.
		size_t GetSize() const;

		// Takes implicit ownership of |h|.
		// |guid| uniquely identifies the shared memory region pointed to by the
		// underlying OS resource. If the HANDLE is associated with another
		// SharedMemoryHandle, the caller must pass the |guid| of that
		// SharedMemoryHandle. Otherwise, the caller should generate a new
		// UnguessableToken.
		// Passing the wrong |size| has no immediate consequence, but may cause errors
		// when trying to map the SharedMemoryHandle at a later point in time.
		SharedMemoryHandle(HANDLE h, size_t size, const base::UnguessableToken& guid);
		HANDLE GetHandle() const;

	private:
		HANDLE handle_ = nullptr;

		// Whether passing this object as a parameter to an IPC message passes
		// ownership of |handle_| to the IPC stack. This is meant to mimic the
		// behavior of the |auto_close| parameter of FileDescriptor. This member only
		// affects attachment-brokered SharedMemoryHandles.
		// Defaults to |false|.
		bool ownership_passes_to_ipc_ = false;
		base::UnguessableToken guid_;

		// The size of the region referenced by the SharedMemoryHandle.
		size_t size_ = 0;
	};

}  // namespace base

