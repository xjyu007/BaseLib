#pragma once

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "compiler_specific.h"
#include "macros.h"
#include "memory/shared_memory_handle.h"
#include "unguessable_token.h"
#include "build_config.h"
#include "win/scoped_handle.h"
#include "win/windows_types.h"

namespace base {
	namespace subtle {

		// Implementation class for shared memory regions.
		//
		// This class does the following:
		//
		// - Wraps and owns a shared memory region platform handle.
		// - Provides a way to allocate a new region of platform shared memory of given
		//   size.
		// - Provides a way to create mapping of the region in the current process'
		//   address space, under special access-control constraints (see Mode).
		// - Provides methods to help transferring the handle across process boundaries.
		// - Holds a 128-bit unique identifier used to uniquely identify the same
		//   kernel region resource across processes (used for memory tracking).
		// - Has a method to retrieve the region's size in bytes.
		//
		// IMPORTANT NOTE: Users should never use this directly, but
		// ReadOnlySharedMemoryRegion, WritableSharedMemoryRegion or
		// UnsafeSharedMemoryRegion since this is an implementation class.
		class BASE_EXPORT PlatformSharedMemoryRegion {
		public:
			// Permission mode of the platform handle. Each mode corresponds to one of the
			// typed shared memory classes:
			//
			// * ReadOnlySharedMemoryRegion: A region that can only create read-only
			// mappings.
			//
			// * WritableSharedMemoryRegion: A region that can only create writable
			// mappings. The region can be demoted to ReadOnlySharedMemoryRegion without
			// the possibility of promoting back to writable.
			//
			// * UnsafeSharedMemoryRegion: A region that can only create writable
			// mappings. The region cannot be demoted to ReadOnlySharedMemoryRegion.
			enum class Mode {
				kReadOnly,  // ReadOnlySharedMemoryRegion
				kWritable,  // WritableSharedMemoryRegion
				kUnsafe,    // UnsafeSharedMemoryRegion
				kMaxValue = kUnsafe
			};

			// Errors that can occur during Shared Memory construction.
			// These match tools/metrics/histograms/enums.xml.
			// This enum is append-only.
			enum class CreateError {
				SUCCESS = 0,
				SIZE_ZERO = 1,
				SIZE_TOO_LARGE = 2,
				INITIALIZE_ACL_FAILURE = 3,
				INITIALIZE_SECURITY_DESC_FAILURE = 4,
				SET_SECURITY_DESC_FAILURE = 5,
				CREATE_FILE_MAPPING_FAILURE = 6,
				REDUCE_PERMISSIONS_FAILURE = 7,
				ALREADY_EXISTS = 8,
			    ALLOCATE_FILE_REGION_FAILURE = 9,
			    FSTAT_FAILURE = 10,
			    INODES_MISMATCH = 11,
			    GET_SHMEM_TEMP_DIR_FAILURE = 12,
			    kMaxValue = GET_SHMEM_TEMP_DIR_FAILURE
			};

			// Platform-specific shared memory type used by this class.
			using PlatformHandle = HANDLE;
			using ScopedPlatformHandle = win::ScopedHandle;

			// The minimum alignment in bytes that any mapped address produced by Map()
			// and MapAt() is guaranteed to have.
			enum { kMapMinimumAlignment = 32 };

			// Creates a new PlatformSharedMemoryRegion with corresponding mode and size.
			// Creating in kReadOnly mode isn't supported because then there will be no
			// way to modify memory content.
			static PlatformSharedMemoryRegion CreateWritable(size_t size);
			static PlatformSharedMemoryRegion CreateUnsafe(size_t size);

			// Returns a new PlatformSharedMemoryRegion that takes ownership of the
			// |handle|. All parameters must be taken from another valid
			// PlatformSharedMemoryRegion instance, e.g. |size| must be equal to the
			// actual region size as allocated by the kernel.
			// Closes the |handle| and returns an invalid instance if passed parameters
			// are invalid.
			static PlatformSharedMemoryRegion Take(ScopedPlatformHandle handle,
			                                       Mode mode,
			                                       size_t size,
			                                       const UnguessableToken& guid);
			// As Take, above, but from a SharedMemoryHandle. This takes ownership of the
			// handle. |mode| must be kUnsafe or kReadOnly; the latter must be used with a
			// handle created with SharedMemoryHandle::GetReadOnlyHandle().
			// TODO(crbug.com/795291): this should only be used while transitioning from
			// the old shared memory API, and should be removed when done.
			static PlatformSharedMemoryRegion TakeFromSharedMemoryHandle(
				const SharedMemoryHandle& handle,
				Mode mode);

			// Default constructor initializes an invalid instance, i.e. an instance that
			// doesn't wrap any valid platform handle.
			PlatformSharedMemoryRegion();

			// Move operations are allowed.
			PlatformSharedMemoryRegion(PlatformSharedMemoryRegion&&);
			PlatformSharedMemoryRegion& operator=(PlatformSharedMemoryRegion&&);

			// Destructor closes the platform handle. Does nothing if the handle is
			// invalid.
			~PlatformSharedMemoryRegion();

			// Passes ownership of the platform handle to the caller. The current instance
			// becomes invalid. It's the responsibility of the caller to close the
			// handle. If the current instance is invalid, ScopedPlatformHandle will also
			// be invalid.
			ScopedPlatformHandle PassPlatformHandle() WARN_UNUSED_RESULT;

			// Returns the platform handle. The current instance keeps ownership of this
			// handle.
			PlatformHandle GetPlatformHandle() const;

			// Whether the platform handle is valid.
			bool IsValid() const;

			// Duplicates the platform handle and creates a new PlatformSharedMemoryRegion
			// with the same |mode_|, |size_| and |guid_| that owns this handle. Returns
			// invalid region on failure, the current instance remains valid.
			// Can be called only in kReadOnly and kUnsafe modes, CHECK-fails if is
			// called in kWritable mode.
			PlatformSharedMemoryRegion Duplicate() const;

			// Converts the region to read-only. Returns whether the operation succeeded.
			// Makes the current instance invalid on failure. Can be called only in
			// kWritable mode, all other modes will CHECK-fail. The object will have
			// kReadOnly mode after this call on success.
			bool ConvertToReadOnly();

			// Converts the region to unsafe. Returns whether the operation succeeded.
			// Makes the current instance invalid on failure. Can be called only in
			// kWritable mode, all other modes will CHECK-fail. The object will have
			// kUnsafe mode after this call on success.
			bool ConvertToUnsafe();

			// Maps |size| bytes of the shared memory region starting with the given
			// |offset| into the caller's address space. |offset| must be aligned to value
			// of |SysInfo::VMAllocationGranularity()|. Fails if requested bytes are out
			// of the region limits.
			// Returns true and sets |memory| and |mapped_size| on success, returns false
			// and leaves output parameters in unspecified state otherwise. The mapped
			// address is guaranteed to have an alignment of at least
			// |kMapMinimumAlignment|.
			bool MapAt(off_t offset,
			           size_t size,
			           void** memory,
			           size_t* mapped_size) const;

			const UnguessableToken& GetGUID() const { return guid_; }

			size_t GetSize() const { return size_; }

			Mode GetMode() const { return mode_; }

		private:
			//FRIEND_TEST_ALL_PREFIXES(PlatformSharedMemoryRegionTest, CreateReadOnlyRegionDeathTest);
			//FRIEND_TEST_ALL_PREFIXES(PlatformSharedMemoryRegionTest, CheckPlatformHandlePermissionsCorrespondToMode);
			static PlatformSharedMemoryRegion Create(Mode mode,
			                                       size_t size
			);

			static bool CheckPlatformHandlePermissionsCorrespondToMode(
				PlatformHandle handle,
				Mode mode,
				size_t size);

			PlatformSharedMemoryRegion(ScopedPlatformHandle handle,
			                           Mode mode,
			                           size_t size,
			                           const UnguessableToken& guid);

			bool MapAtInternal(off_t offset,
			                   size_t size,
			                   void** memory,
			                   size_t* mapped_size) const;

			ScopedPlatformHandle handle_;
			Mode mode_ = Mode::kReadOnly;
			size_t size_ = 0;
			UnguessableToken guid_;

			DISALLOW_COPY_AND_ASSIGN(PlatformSharedMemoryRegion);
		};

	}  // namespace subtle
}  // namespace base
