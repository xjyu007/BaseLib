// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "process/kill.h"

#include <algorithm>

#include <Windows.h>
#include <cstdint>

#include "logging.h"
#include "process/memory.h"
#include "process/process_iterator.h"

namespace base {

	TerminationStatus GetTerminationStatus(ProcessHandle handle, int* exit_code) {
		DCHECK(exit_code);

		DWORD tmp_exit_code = 0;

		if (!::GetExitCodeProcess(handle, &tmp_exit_code)) {
			DPLOG(FATAL) << "GetExitCodeProcess() failed";

			// This really is a random number.  We haven't received any
			// information about the exit code, presumably because this
			// process doesn't have permission to get the exit code, or
			// because of some other cause for GetExitCodeProcess to fail
			// (MSDN docs don't give the possible failure error codes for
			// this function, so it could be anything).  But we don't want
			// to leave exit_code uninitialized, since that could cause
			// random interpretations of the exit code.  So we assume it
			// terminated "normally" in this case.
			* exit_code = win::kNormalTerminationExitCode;

			// Assume the child has exited normally if we can't get the exit
			// code.
			return TERMINATION_STATUS_NORMAL_TERMINATION;
		}
		if (tmp_exit_code == STILL_ACTIVE) {
			const auto wait_result = WaitForSingleObject(handle, 0);
			if (wait_result == WAIT_TIMEOUT) {
				*exit_code = wait_result;
				return TERMINATION_STATUS_STILL_RUNNING;
			}

			if (wait_result == WAIT_FAILED) {
				DPLOG(ERROR) << "WaitForSingleObject() failed";
			}
			else {
				DCHECK_EQ(WAIT_OBJECT_0, wait_result);

				// Strange, the process used 0x103 (STILL_ACTIVE) as exit code.
				NOTREACHED();
			}

			return TERMINATION_STATUS_ABNORMAL_TERMINATION;
		}

		*exit_code = tmp_exit_code;

		switch (tmp_exit_code) {
		case win::kNormalTerminationExitCode:
			return TERMINATION_STATUS_NORMAL_TERMINATION;
		case win::kDebuggerInactiveExitCode:    // STATUS_DEBUGGER_INACTIVE.
		case win::kKeyboardInterruptExitCode:   // Control-C/end session.
		case win::kDebuggerTerminatedExitCode:  // Debugger terminated process.
		case win::kProcessKilledExitCode:       // Task manager kill.
			return TERMINATION_STATUS_PROCESS_WAS_KILLED;
		case win::kSandboxFatalMemoryExceeded:  // Terminated process due to
												// exceeding the sandbox job
												// object memory limits.
		case win::kOomExceptionCode:            // Ran out of memory.
			return TERMINATION_STATUS_OOM;
		default:
			// All other exit codes indicate crashes.
			return TERMINATION_STATUS_PROCESS_CRASHED;
		}
	}

	bool WaitForProcessesToExit(const FilePath::StringType& executable_name,
								TimeDelta wait,
								const ProcessFilter* filter) {
		auto result = true;
		const auto start_time = GetTickCount();
#undef max
		NamedProcessIterator iter(executable_name, filter);
		for (auto entry = iter.NextProcessEntry(); entry;
			entry = iter.NextProcessEntry()) {
			const auto remaining_wait = static_cast<DWORD>(
				std::max(static_cast<int64_t>(0),
					wait.InMilliseconds() - (GetTickCount() - start_time)));
			const auto process = OpenProcess(SYNCHRONIZE,
				FALSE,
				entry->th32ProcessID);
			const auto wait_result = WaitForSingleObject(process, remaining_wait);
			CloseHandle(process);
			result &= (wait_result == WAIT_OBJECT_0);
		}

		return result;
	}

	bool CleanupProcesses(const FilePath::StringType& executable_name,
						  TimeDelta wait,
						  int exit_code,
						  const ProcessFilter* filter) {
		if (WaitForProcessesToExit(executable_name, wait, filter))
			return true;
		KillProcesses(executable_name, exit_code, filter);
		return false;
	}

}  // namespace base
