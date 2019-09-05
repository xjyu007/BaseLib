// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains functions for launching subprocesses.

#pragma once

#include <string>
#include <vector>

#include "base_export.h"
#include "environment.h"
#include "process/process.h"
#include "process/process_handle.h"
#include "files/file_path.h"

namespace base {

	class CommandLine;

	typedef std::vector<HANDLE> HandlesToInheritVector;

	// Options for launching a subprocess that are passed to LaunchProcess().
	// The default constructor constructs the object with default options.
	struct BASE_EXPORT LaunchOptions {
		LaunchOptions();
		LaunchOptions(const LaunchOptions&);
		~LaunchOptions();

		// If true, wait for the process to complete.
		bool wait = false;

		// If not empty, change to this directory before executing the new process.
		base::FilePath current_directory;

		bool start_hidden = false;

		// Sets STARTF_FORCEOFFFEEDBACK so that the feedback cursor is forced off
		// while the process is starting.
		bool feedback_cursor_off = false;

		// Windows can inherit handles when it launches child processes.
		// See https://blogs.msdn.microsoft.com/oldnewthing/20111216-00/?p=8873
		// for a good overview of Windows handle inheritance.
		//
		// Implementation note: it might be nice to implement in terms of
		// base::Optional<>, but then the natural default state (vector not present)
		// would be "all inheritable handles" while we want "no inheritance."
		enum class Inherit {
			// Only those handles in |handles_to_inherit| vector are inherited. If the
			// vector is empty, no handles are inherited. The handles in the vector must
			// all be inheritable.
			kSpecific,

			// All handles in the current process which are inheritable are inherited.
			// In production code this flag should be used only when running
			// short-lived, trusted binaries, because open handles from other libraries
			// and subsystems will leak to the child process, causing errors such as
			// open socket hangs. There are also race conditions that can cause handle
			// over-sharing.
			//
			// |handles_to_inherit| must be null.
			//
			// DEPRECATED. THIS SHOULD NOT BE USED. Explicitly map all handles that
			// need to be shared in new code.
			// TODO(brettw) bug 748258: remove this.
			kAll
		};
		Inherit inherit_mode = Inherit::kSpecific;
		HandlesToInheritVector handles_to_inherit{};

		// If non-null, runs as if the user represented by the token had launched it.
		// Whether the application is visible on the interactive desktop depends on
		// the token belonging to an interactive logon session.
		//
		// To avoid hard to diagnose problems, when specified this loads the
		// environment variables associated with the user and if this operation fails
		// the entire call fails as well.
		UserTokenHandle as_user = nullptr;

		// If true, use an empty string for the desktop name.
		bool empty_desktop_name = false;

		// If non-null, launches the application in that job object. The process will
		// be terminated immediately and LaunchProcess() will fail if assignment to
		// the job object fails.
		HANDLE job_handle = nullptr;

		// Handles for the redirection of stdin, stdout and stderr. The caller should
		// either set all three of them or none (i.e. there is no way to redirect
		// stderr without redirecting stdin).
		//
		// The handles must be inheritable. Pseudo handles are used when stdout and
		// stderr redirect to the console. In that case, GetFileType() will return
		// FILE_TYPE_CHAR and they're automatically inherited by child processes. See
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682075.aspx
		// Otherwise, the caller must ensure that the |inherit_mode| and/or
		// |handles_to_inherit| set so that the handles are inherited.
		HANDLE stdin_handle = nullptr;
		HANDLE stdout_handle = nullptr;
		HANDLE stderr_handle = nullptr;

		// If set to true, ensures that the child process is launched with the
		// CREATE_BREAKAWAY_FROM_JOB flag which allows it to breakout of the parent
		// job if any.
		bool force_breakaway_from_job_ = false;

		// If set to true, permission to bring windows to the foreground is passed to
		// the launched process if the current process has such permission.
		bool grant_foreground_privilege = false;

		// Set/unset environment variables. These are applied on top of the parent
		// process environment.  Empty (the default) means to inherit the same
		// environment. See internal::AlterEnvironment().
		EnvironmentMap environment;

		// Clear the environment for the new process before processing changes from
		// |environment|.
		bool clear_environment = false;

	};

	// Launch a process via the command line |cmdline|.
	// See the documentation of LaunchOptions for details on |options|.
	//
	// Returns a valid Process upon success.
	//
	// Unix-specific notes:
	// - All file descriptors open in the parent process will be closed in the
	//   child process except for any preserved by options::fds_to_remap, and
	//   stdin, stdout, and stderr. If not remapped by options::fds_to_remap,
	//   stdin is reopened as /dev/null, and the child is allowed to inherit its
	//   parent's stdout and stderr.
	// - If the first argument on the command line does not contain a slash,
	//   PATH will be searched.  (See man execvp.)
	BASE_EXPORT Process LaunchProcess(const CommandLine& cmdline,
		const LaunchOptions& options);

	// Windows-specific LaunchProcess that takes the command line as a
	// string.  Useful for situations where you need to control the
	// command line arguments directly, but prefer the CommandLine version
	// if launching Chrome itself.
	//
	// The first command line argument should be the path to the process,
	// and don't forget to quote it.
	//
	// Example (including literal quotes)
	//  cmdline = "c:\windows\explorer.exe" -foo "c:\bar\"
	BASE_EXPORT Process LaunchProcess(const std::wstring& cmdline,
									  const LaunchOptions& options);

	// Launches a process with elevated privileges.  This does not behave exactly
	// like LaunchProcess as it uses ShellExecuteEx instead of CreateProcess to
	// create the process.  This means the process will have elevated privileges
	// and thus some common operations like OpenProcess will fail. Currently the
	// only supported LaunchOptions are |start_hidden| and |wait|.
	BASE_EXPORT Process LaunchElevatedProcess(const CommandLine& cmdline,
		const LaunchOptions& options);

	// Set |job_object|'s JOBOBJECT_EXTENDED_LIMIT_INFORMATION
	// BasicLimitInformation.LimitFlags to |limit_flags|.
	BASE_EXPORT bool SetJobObjectLimitFlags(HANDLE job_object, DWORD limit_flags);

	// Output multi-process printf, cout, cerr, etc to the cmd.exe console that ran
	// chrome. This is not thread-safe: only call from main thread.
	BASE_EXPORT void RouteStdioToConsole(bool create_console_if_not_found);

	// Executes the application specified by |cl| and wait for it to exit. Stores
	// the output (stdout) in |output|. Redirects stderr to /dev/null. Returns true
	// on success (application launched and exited cleanly, with exit code
	// indicating success).
	BASE_EXPORT bool GetAppOutput(const CommandLine& cl, std::string* output);

	// Like GetAppOutput, but also includes stderr.
	BASE_EXPORT bool GetAppOutputAndError(const CommandLine& cl,
		std::string* output);

	// A version of |GetAppOutput()| which also returns the exit code of the
	// executed command. Returns true if the application runs and exits cleanly. If
	// this is the case the exit code of the application is available in
	// |*exit_code|.
	BASE_EXPORT bool GetAppOutputWithExitCode(const CommandLine& cl,
		std::string* output, int* exit_code);

	// A Windows-specific version of GetAppOutput that takes a command line string
	// instead of a CommandLine object. Useful for situations where you need to
	// control the command line arguments directly.
	BASE_EXPORT bool GetAppOutput(const std::wstring_view& cl, std::string* output);

	// If supported on the platform, and the user has sufficent rights, increase
	// the current process's scheduling priority to a high priority.
	BASE_EXPORT void RaiseProcessToHighPriority();

	// Creates a LaunchOptions object suitable for launching processes in a test
	// binary. This should not be called in production/released code.
	BASE_EXPORT LaunchOptions LaunchOptionsForTest();

}  // namespace base
