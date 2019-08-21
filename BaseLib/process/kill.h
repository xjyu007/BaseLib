#pragma once

// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains routines to kill processes and get the exit code and
// termination status.

#include "files/file_path.h"
#include "process/process.h"
#include "process/process_handle.h"
#include "time/time.h"

namespace base {

	class ProcessFilter;

	namespace win {

		// See definition in sandbox/win/src/sandbox_types.h
		const DWORD kSandboxFatalMemoryExceeded = 7012;

		// Exit codes with special meanings on Windows.
		const DWORD kNormalTerminationExitCode = 0;
		const DWORD kDebuggerInactiveExitCode = 0xC0000354;
		const DWORD kKeyboardInterruptExitCode = 0xC000013A;
		const DWORD kDebuggerTerminatedExitCode = 0x40010004;

		// This exit code is used by the Windows task manager when it kills a
		// process.  It's value is obviously not that unique, and it's
		// surprising to me that the task manager uses this value, but it
		// seems to be common practice on Windows to test for it as an
		// indication that the task manager has killed something if the
		// process goes away.
		const DWORD kProcessKilledExitCode = 1;

	}  // namespace win

// Return status values from GetTerminationStatus.  Don't use these as
// exit code arguments to KillProcess*(), use platform/application
// specific values instead.
	enum TerminationStatus {
		TERMINATION_STATUS_NORMAL_TERMINATION,   // zero exit status
		TERMINATION_STATUS_ABNORMAL_TERMINATION, // non-zero exit status
		TERMINATION_STATUS_PROCESS_WAS_KILLED,   // e.g. SIGKILL or task manager kill
		TERMINATION_STATUS_PROCESS_CRASHED,      // e.g. Segmentation fault
		TERMINATION_STATUS_STILL_RUNNING,        // child hasn't exited yet
		TERMINATION_STATUS_LAUNCH_FAILED,        // child process never launched
		TERMINATION_STATUS_OOM,                  // Process died due to oom
		TERMINATION_STATUS_MAX_ENUM
	};

	// Attempts to kill all the processes on the current machine that were launched
	// from the given executable name, ending them with the given exit code.  If
	// filter is non-null, then only processes selected by the filter are killed.
	// Returns true if all processes were able to be killed off, false if at least
	// one couldn't be killed.
	BASE_EXPORT bool KillProcesses(const FilePath::StringType& executable_name, int exit_code, const ProcessFilter* filter);

	// Get the termination status of the process by interpreting the
	// circumstances of the child process' death. |exit_code| is set to
	// the status returned by waitpid() on POSIX, and from GetExitCodeProcess() on
	// Windows, and may not be null.  Note that on Linux, this function
	// will only return a useful result the first time it is called after
	// the child exits (because it will reap the child and the information
	// will no longer be available).
	BASE_EXPORT TerminationStatus GetTerminationStatus(ProcessHandle handle, int* exit_code);

	// Registers |process| to be asynchronously monitored for termination, forcibly
	// terminated if necessary, and reaped on exit. The caller should have signalled
	// |process| to exit before calling this API. The API will allow a couple of
	// seconds grace period before forcibly terminating |process|.
	// TODO(https://crbug.com/806451): The Mac implementation currently blocks the
	// calling thread for up to two seconds.
	BASE_EXPORT void EnsureProcessTerminated(Process process);

	// These are only sparingly used, and not needed on Fuchsia. They could be
	// implemented if necessary.
	// Wait for all the processes based on the named executable to exit.  If filter
	// is non-null, then only processes selected by the filter are waited on.
	// Returns after all processes have exited or wait_milliseconds have expired.
	// Returns true if all the processes exited, false otherwise.
	BASE_EXPORT bool WaitForProcessesToExit(const FilePath::StringType& executable_name, base::TimeDelta wait, const ProcessFilter* filter);

	// Waits a certain amount of time (can be 0) for all the processes with a given
	// executable name to exit, then kills off any of them that are still around.
	// If filter is non-null, then only processes selected by the filter are waited
	// on.  Killed processes are ended with the given exit code.  Returns false if
	// any processes needed to be killed, true if they all exited cleanly within
	// the wait_milliseconds delay.
	BASE_EXPORT bool CleanupProcesses(const FilePath::StringType& executable_name, base::TimeDelta wait, int exit_code, const ProcessFilter* filter);

}  // namespace base
