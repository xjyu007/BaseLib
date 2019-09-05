// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>

#include "base_export.h"
#include "files/file_path.h"

#include "win/windows_types.h"

namespace base {

	// ProcessHandle is a platform specific type which represents the underlying OS
	// handle to a process.
	// ProcessId is a number which identifies the process in the OS.
	typedef HANDLE ProcessHandle;
	typedef DWORD ProcessId;
	typedef HANDLE UserTokenHandle;
	const ProcessHandle kNullProcessHandle = NULL;
	const ProcessId kNullProcessId = 0;

	// To print ProcessIds portably use CrPRIdPid (based on PRIuS and friends from
	// C99 and format_macros.h) like this:
	// base::StringPrintf("PID is %" CrPRIdPid ".\n", pid);
#define CrPRIdPid "ld"

	class UniqueProcId {
	public:
		explicit UniqueProcId(ProcessId value) : value_(value) {}
		UniqueProcId(const UniqueProcId& other) = default;
		UniqueProcId& operator=(const UniqueProcId& other) = default;

		// Returns the process PID. WARNING: On some platforms, the pid may not be
		// valid within the current process sandbox.
		[[nodiscard]] ProcessId GetUnsafeValue() const { return value_; }

		bool operator==(const UniqueProcId& other) const {
			return value_ == other.value_;
		}

		bool operator!=(const UniqueProcId& other) const {
			return value_ != other.value_;
		}

		bool operator<(const UniqueProcId& other) const {
			return value_ < other.value_;
		}

		bool operator<=(const UniqueProcId& other) const {
			return value_ <= other.value_;
		}

		bool operator>(const UniqueProcId& other) const {
			return value_ > other.value_;
		}

		bool operator>=(const UniqueProcId& other) const {
			return value_ >= other.value_;
		}

	private:
		ProcessId value_;
	};

	std::ostream& operator<<(std::ostream& os, const UniqueProcId& obj);

	// Returns the id of the current process.
	// Note that on some platforms, this is not guaranteed to be unique across
	// processes (use GetUniqueIdForProcess if uniqueness is required).
	BASE_EXPORT ProcessId GetCurrentProcId();

	// Returns a unique ID for the current process. The ID will be unique across all
	// currently running processes within the chrome session, but IDs of terminated
	// processes may be reused.
	BASE_EXPORT UniqueProcId GetUniqueIdForProcess();

	// Returns the ProcessHandle of the current process.
	BASE_EXPORT ProcessHandle GetCurrentProcessHandle();

	// Returns the process ID for the specified process. This is functionally the
	// same as Windows' GetProcessId(), but works on versions of Windows before Win
	// XP SP1 as well.
	// DEPRECATED. New code should be using Process::Pid() instead.
	// Note that on some platforms, this is not guaranteed to be unique across
	// processes.
	BASE_EXPORT ProcessId GetProcId(ProcessHandle process);

	// Returns the ID for the parent of the given process. Not available on Fuchsia.
	// Returning a negative value indicates an error, such as if the |process| does
	// not exist. Returns 0 when |process| has no parent process.
	BASE_EXPORT ProcessId GetParentProcessId(ProcessHandle process);

} // namespace base