// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "logging.h"

#include <cstdint>

#include "pending_task.h"
#include "stl_util.h"
#include "task/common/task_annotator.h"
#include "trace_event/trace_event.h"
#include "build_config.h"

#include <io.h>
#include <Windows.h>
typedef HANDLE FileHandle;
typedef HANDLE MutexHandle;
// Windows warns on using write().  It prefers _write().
#define write(fd, buf, count) _write(fd, buf, static_cast<unsigned int>(count))
// Windows doesn't define STDERR_FILENO.  Define it here.
#define STDERR_FILENO 2

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

#include "base_switches.h"
#include "callback.h"
#include "command_line.h"
#include "containers/stack.h"
#include "debug_/activity_tracker.h"
#include "debug_/alias.h"
#include "debug_/debugger.h"
#include "debug_/stack_trace.h"
#include "debug_/task_trace.h"
#include "no_destructor.h"
#include "posix/eintr_wrapper.h"
#include "strings/string_util.h"
#include "strings/stringprintf.h"
#include "strings/utf_string_conversions.h"
#include "threading/platform_thread.h"
#include "vlog.h"

#include "win/win_util.h"

namespace logging {
	namespace {
		VlogInfo* g_vlog_info = nullptr;
		VlogInfo* g_vlog_info_prev = nullptr;

		const char* const log_severity_names[] = { "INFO", "WARNING", "ERROR", "FATAL" };
		static_assert(LOG_NUM_SEVERITIES == base::size(log_severity_names), 
					  "Incorrect number of log_severity_names");

		const char* log_severity_name(int severity) {
			if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
				return log_severity_names[severity];
			return "UNKNOWN";
		}

		int g_min_log_level = 0;

		// Specifies the process' logging sink(s), represented as a combination of
		// LoggingDestination values joined by bitwise OR.
		int g_logging_destination = LOG_DEFAULT;

		// For LOG_ERROR and above, always print to stderr.
		const int kAlwaysPrintErrorLevel = LOG_ERROR;

		// Which log file to use? This is initialized by InitLogging or
		// will be lazily initialized to the default value when it is
		// first needed.
		using PathString = base::FilePath::StringType;
		PathString* g_log_file_name = nullptr;

		// This file is lazily opened and the handle may be nullptr
		FileHandle g_log_file = nullptr;

		// What should be prepended to each message?
		bool g_log_process_id = false;
		bool g_log_thread_id = false;
		bool g_log_timestamp = true;
		bool g_log_tickcount = false;
		const char* g_log_prefix = nullptr;

		// Should we pop up fatal debug messages in a dialog?
		bool show_error_dialogs = false;

		// An assert handler override specified by the client to be called instead of
		// the debug message dialog and process termination. Assert handlers are stored
		// in stack to allow overriding and restoring.
		base::stack<LogAssertHandlerFunction>& GetLogAssertHandlerStack() {
			static base::NoDestructor<base::stack<LogAssertHandlerFunction>> instance;
			return *instance;
		}

		// A log message handler that gets notified of every log message we process.
		LogMessageHandlerFunction log_message_handler = nullptr;

		uint64_t TickCount() {
			return GetTickCount();
		}

		void DeleteFilePath(const PathString& log_name) {
			DeleteFile(base::as_wcstr(log_name));
		}

		PathString GetDefaultLogFile() {
			// On Windows we use the same path as the exe.
			wchar_t module_name[MAX_PATH];
			GetModuleFileName(nullptr, base::as_writable_wcstr(module_name), MAX_PATH);

			PathString log_name = module_name;
			const auto last_backslash = log_name.rfind('\\', log_name.size());
			if (last_backslash != PathString::npos)
				log_name.erase(last_backslash + 1);
			log_name += L"debug.log";
			return log_name;
		}

		// Called by logging functions to ensure that |g_log_file| is initialized
		// and can be used for writing. Returns false if the file could not be
		// initialized. |g_log_file| will be nullptr in this case.
		bool InitializeLogFileHandle() {
			if (g_log_file)
				return true;

			if (!g_log_file_name) {
				// Nobody has called InitLogging to specify a debug log file, so here we
				// initialize the log file name to a default.
				g_log_file_name = new PathString(GetDefaultLogFile());
			}

			if ((g_logging_destination & LOG_TO_FILE) == 0)
				return true;

			// The FILE_APPEND_DATA access mask ensures that the file is atomically
			// appended to across accesses from multiple threads.
			// https://msdn.microsoft.com/en-us/library/windows/desktop/aa364399(v=vs.85).aspx
			// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
			g_log_file = CreateFile(base::as_wcstr(*g_log_file_name), FILE_APPEND_DATA,
			                        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (g_log_file == INVALID_HANDLE_VALUE || g_log_file == nullptr) {
				// We are intentionally not using FilePath or FileUtil here to reduce the
				// dependencies of the logging implementation. For e.g. FilePath and
				// FileUtil depend on shell32 and user32.dll. This is not acceptable for
				// some consumers of base logging like chrome_elf, etc.
				// Please don't change the code below to use FilePath.
				// try the current directory
				wchar_t system_buffer[MAX_PATH];
				system_buffer[0] = 0;
				const auto len = ::GetCurrentDirectory(base::size(system_buffer), 
													   base::as_writable_wcstr(system_buffer));
				if (len == 0 || len > base::size(system_buffer))
					return false;

				*g_log_file_name = system_buffer;
				// Append a trailing backslash if needed.
				if (g_log_file_name->back() != L'\\')
					* g_log_file_name += L"\\";
				*g_log_file_name += L"debug.log";

				g_log_file = CreateFile(base::as_wcstr(*g_log_file_name), FILE_APPEND_DATA,
				                     FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, 
									 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				if (g_log_file == INVALID_HANDLE_VALUE || g_log_file == nullptr) {
					g_log_file = nullptr;
					return false;
				}
			}
			return true;
		}

		void CloseFile(FileHandle log) {
			CloseHandle(log);
		}

		void CloseLogFileUnlocked() {
			if (!g_log_file)
				return;

			CloseFile(g_log_file);
			g_log_file = nullptr;

			// If we initialized logging via an externally-provided file descriptor, we
			// won't have a log path set and shouldn't try to reopen the log file.
			if (!g_log_file_name)
				g_logging_destination &= ~LOG_TO_FILE;
		}

	}  // namespace

#if defined(DCHECK_IS_CONFIGURABLE)
// In DCHECK-enabled Chrome builds, allow the meaning of LOG_DCHECK to be
// determined at run-time. We default it to INFO, to avoid it triggering
// crashes before the run-time has explicitly chosen the behaviour.
	BASE_EXPORT logging::LogSeverity LOG_DCHECK = LOG_INFO;
#endif  // defined(DCHECK_IS_CONFIGURABLE)

	// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
	// an object of the correct type on the LHS of the unused part of the ternary
	// operator.
	std::ostream* g_swallow_stream;

	bool BaseInitLoggingImpl(const LoggingSettings& settings) {
		base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
		// Don't bother initializing |g_vlog_info| unless we use one of the
		// vlog switches.
		if (command_line->HasSwitch(switches::kV) ||
		    command_line->HasSwitch(switches::kVModule)) {
			// NOTE: If |g_vlog_info| has already been initialized, it might be in use
			// by another thread. Don't delete the old VLogInfo, just create a second
			// one. We keep track of both to avoid memory leak warnings.
			CHECK(!g_vlog_info_prev);
			g_vlog_info_prev = g_vlog_info;

			g_vlog_info = 
				new VlogInfo(command_line->GetSwitchValueASCII(switches::kV),
							 command_line->GetSwitchValueASCII(switches::kVModule),
							 &g_min_log_level);
		}

		g_logging_destination = settings.logging_dest;

		// ignore file options unless logging to file is set.
		if ((g_logging_destination & LOG_TO_FILE) == 0)
			return true;

		// Calling InitLogging twice or after some log call has already opened the
		// default log file will re-initialize to the new options.
		CloseLogFileUnlocked();

		if (!g_log_file_name)
			g_log_file_name = new PathString();
		*g_log_file_name = settings.log_file_path;
		if (settings.delete_old == DELETE_OLD_LOG_FILE)
			DeleteFilePath(*g_log_file_name);

		return InitializeLogFileHandle();
	}

	void SetMinLogLevel(int level) {
#undef min
		g_min_log_level = std::min(LOG_FATAL, level);
	}

	int GetMinLogLevel() {
		return g_min_log_level;
	}

	bool ShouldCreateLogMessage(int severity) {
		if (severity < g_min_log_level)
			return false;

		// Return true here unless we know ~LogMessage won't do anything. Note that
		return g_logging_destination != LOG_NONE || log_message_handler ||
			severity >= kAlwaysPrintErrorLevel;
	}

	// Returns true when LOG_TO_STDERR flag is set, or |severity| is high.
	// If |severity| is high then true will be returned when no log destinations are
	// set, or only LOG_TO_FILE is set, since that is useful for local development
	// and debugging.
	bool ShouldLogToStderr(int severity) {
		if (g_logging_destination & LOG_TO_STDERR)
			return true;
		if (severity >= kAlwaysPrintErrorLevel)
			return (g_logging_destination & ~LOG_TO_FILE) == LOG_NONE;
		return false;
	}

	int GetVlogVerbosity() {
#undef max
		return std::max(-1, LOG_INFO - GetMinLogLevel());
	}

	int GetVlogLevelHelper(const char* file, size_t N) {
		DCHECK_GT(N, 0U);
		// Note: |g_vlog_info| may change on a different thread during startup
		// (but will always be valid or nullptr).
		VlogInfo* vlog_info = g_vlog_info;
		return vlog_info ?
			vlog_info->GetVlogLevel(std::string(file, N - 1)) :
			GetVlogVerbosity();
	}

	void SetLogItems(bool enable_process_id, bool enable_thread_id,
		bool enable_timestamp, bool enable_tickcount) {
		g_log_process_id = enable_process_id;
		g_log_thread_id = enable_thread_id;
		g_log_timestamp = enable_timestamp;
		g_log_tickcount = enable_tickcount;
	}

	void SetLogPrefix(const char* prefix) {
		DCHECK(!prefix || 
				base::ContainsOnlyChars(prefix, "abcdefghijklmnopqrstuvwxyz"));
		g_log_prefix = prefix;
	}

	void SetShowErrorDialogs(bool enable_dialogs) {
		show_error_dialogs = enable_dialogs;
	}

	ScopedLogAssertHandler::ScopedLogAssertHandler(
		LogAssertHandlerFunction handler) {
		GetLogAssertHandlerStack().push(std::move(handler));
	}

	ScopedLogAssertHandler::~ScopedLogAssertHandler() {
		GetLogAssertHandlerStack().pop();
	}

	void SetLogMessageHandler(LogMessageHandlerFunction handler) {
		log_message_handler = handler;
	}

	LogMessageHandlerFunction GetLogMessageHandler() {
		return log_message_handler;
	}

	// Explicit instantiations for commonly used comparisons.
	template std::string* MakeCheckOpString<int, int>(
		const int&, const int&, const char* names);
	template std::string* MakeCheckOpString<unsigned long, unsigned long>(
		const unsigned long&, const unsigned long&, const char* names);
	template std::string* MakeCheckOpString<unsigned long, unsigned int>(
		const unsigned long&, const unsigned int&, const char* names);
	template std::string* MakeCheckOpString<unsigned int, unsigned long>(
		const unsigned int&, const unsigned long&, const char* names);
	template std::string* MakeCheckOpString<std::string, std::string>(
		const std::string&, const std::string&, const char* name);

	void MakeCheckOpValueString(std::ostream* os, std::nullptr_t p) {
		(*os) << "nullptr";
	}

#if !defined(NDEBUG)
	// Displays a message box to the user with the error message in it.
	// Used for fatal messages, where we close the app simultaneously.
	// This is for developers only; we don't use this in circumstances
	// (like release builds) where users could see it, since users don't
	// understand these messages anyway.
	void DisplayDebugMessageInDialog(const std::string& str) {
		if (str.empty())
			return;

		if (!show_error_dialogs)
			return;

		// We intentionally don't implement a dialog on other platforms.
		// You can just look at stderr.
		if (base::win::IsUser32AndGdi32Available()) {
			MessageBoxW(nullptr, base::as_wcstr(base::UTF8ToWide(str)), L"Fatal error",
			MB_OK | MB_ICONHAND | MB_TOPMOST);
		} else {
			OutputDebugStringW(base::as_wcstr(base::UTF8ToWide(str)));
		}
	}
#endif  // !defined(NDEBUG)

	LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
	    : severity_(severity), file_(file), line_(line) {
		Init(file, line);
	}

	LogMessage::LogMessage(const char* file, int line, const char* condition)
	    : severity_(LOG_FATAL), file_(file), line_(line) {
		Init(file, line);
		stream_ << "Check failed: " << condition << ". ";
	}

	LogMessage::LogMessage(const char* file, int line, std::string* result)
	    : severity_(LOG_FATAL), file_(file), line_(line) {
		Init(file, line);
		stream_ << "Check failed: " << *result;
		delete result;
	}

	LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
	                       std::string* result)
	    : severity_(severity), file_(file), line_(line) {
		Init(file, line);
		stream_ << "Check failed: " << *result;
		delete result;
	}

	LogMessage::~LogMessage() {
		const auto stack_start = static_cast<size_t>(stream_.tellp());
		if (severity_ == LOG_FATAL && !base::debug::BeingDebugged()) {
			// Include a stack trace on a fatal, unless a debugger is attached.
			const base::debug::StackTrace stack_trace;
			stream_ << std::endl;  // Newline to separate from log message.
			stack_trace.OutputToStream(&stream_);
			const base::debug::TaskTrace task_trace;
			if (!task_trace.empty())
				task_trace.OutputToStream(&stream_);

			// Include the IPC context, if any.
			// TODO(chrisha): Integrate with symbolization once those tools exist!
			const auto* task = base::TaskAnnotator::CurrentTaskForThread();
			if (task && task->ipc_hash) {
				stream_ << "IPC message handler context: "
						<< base::StringPrintf("0x%08X", task->ipc_hash) << std::endl;
			}
		}
		stream_ << std::endl;
		std::string str_newline(stream_.str());
		//TRACE_LOG_MESSAGE(
		//	file_, std::string_view(str_newline).substr(message_start_), line_);

		// Give any log message handler first dibs on the message.
		if (log_message_handler &&
			log_message_handler(severity_, file_, line_,
								message_start_, str_newline)) {
			// The handler took care of it, no further processing.
			return;
		}

		if ((g_logging_destination & LOG_TO_SYSTEM_DEBUG_LOG) != 0) {
			OutputDebugStringA(str_newline.c_str());
  		}

		if (ShouldLogToStderr(severity_)) {
			ignore_result(fwrite(str_newline.data(), str_newline.size(), 1, stderr));
			fflush(stderr);
		}

		if ((g_logging_destination & LOG_TO_FILE) != 0) {
			// We can have multiple threads and/or processes, so try to prevent them
			// from clobbering each other's writes.
			// If the client app did not call InitLogging, and the lock has not
			// been created do it now. We do this on demand, but if two threads try
			// to do this at the same time, there will be a race condition to create
			// the lock. This is why InitLogging should be called from the main
			// thread at the beginning of execution.
			if (InitializeLogFileHandle()) {
				DWORD num_written;
				WriteFile(g_log_file,
					static_cast<const void*>(str_newline.c_str()),
					static_cast<DWORD>(str_newline.length()),
					&num_written,
					nullptr);
			}
		}

		if (severity_ == LOG_FATAL) {
			// Write the log message to the global activity tracker, if running.
			const auto tracker = 
				base::debug::GlobalActivityTracker::Get();
			if (tracker)
				tracker->RecordLogMessage(str_newline);

			// Ensure the first characters of the string are on the stack so they
			// are contained in minidumps for diagnostic purposes. We place start
			// and end marker values at either end, so we can scan captured stacks
			// for the data easily.
			struct {
				uint32_t start_marker = 0xbedead01;
				char data[1024]{};
				uint32_t end_marker = 0x5050dead;
			} str_stack;
			base::strlcpy(str_stack.data, str_newline.data(), 
						  base::size(str_stack.data));
			base::debug::Alias(&str_stack);

		    if (!GetLogAssertHandlerStack().empty()) {
			    const auto log_assert_handler =
		          GetLogAssertHandlerStack().top();

				if (log_assert_handler) {
					log_assert_handler.Run(
						file_, line_,
						std::string_view(str_newline.c_str() + message_start_, 
										 stack_start - message_start_),
						std::string_view(str_newline.c_str() + stack_start));
				}
			} else {
				// Don't use the string with the newline, get a fresh version to send to
				// the debug message process. We also don't display assertions to the
				// user in release mode. The enduser can't do anything with this
				// information, and displaying message boxes when the application is
				// hosed can cause additional problems.
#ifndef NDEBUG
				if (!base::debug::BeingDebugged()) {
					// Displaying a dialog is unnecessary when debugging and can complicate
					// debugging.
					DisplayDebugMessageInDialog(stream_.str());
				}
#endif
				// Crash the process to generate a dump.
#if defined(OFFICIAL_BUILD) && defined(NDEBUG)
				IMMEDIATE_CRASH();
#else
				base::debug::BreakDebugger();
#endif
			}
		}
	}

	// writes the common header info to the stream
	void LogMessage::Init(const char* file, int line) {
		std::string_view filename(file);
		const auto last_slash_pos = filename.find_last_of("\\/");
		if (last_slash_pos != std::string_view::npos)
			filename.remove_prefix(last_slash_pos + 1);

		// TODO(darin): It might be nice if the columns were fixed width.

		stream_ << '[';
		if (g_log_prefix)
			stream_ << g_log_prefix << ':';
		if (g_log_process_id)
			stream_ << base::GetUniqueIdForProcess() << ':';
		if (g_log_thread_id)
			stream_ << base::PlatformThread::CurrentId() << ':';
		if (g_log_timestamp) {
			SYSTEMTIME local_time;
			GetLocalTime(&local_time);
			stream_ << std::setfill('0')
				<< std::setw(2) << local_time.wMonth
				<< std::setw(2) << local_time.wDay
				<< '/'
				<< std::setw(2) << local_time.wHour
				<< std::setw(2) << local_time.wMinute
				<< std::setw(2) << local_time.wSecond
				<< '.'
				<< std::setw(3)
				<< local_time.wMilliseconds
				<< ':';
		}
		if (g_log_tickcount)
			stream_ << TickCount() << ':';
		if (severity_ >= 0)
			stream_ << log_severity_name(severity_);
		else
			stream_ << "VERBOSE" << -severity_;

		stream_ << ":" << filename << "(" << line << ")] ";
		message_start_ = stream_.str().length();
	}

	// This has already been defined in the header, but defining it again as DWORD
	// ensures that the type used in the header is equivalent to DWORD. If not,
	// the redefinition is a compile error.
	typedef DWORD SystemErrorCode;

	SystemErrorCode GetLastSystemErrorCode() {
		return ::GetLastError();
	}

	BASE_EXPORT std::string SystemErrorCodeToString(SystemErrorCode error_code) {
		const auto kErrorMessageBufferSize = 256;
		char msgbuf[kErrorMessageBufferSize];
		const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
		const auto len = FormatMessageA(flags, nullptr, error_code, 0, msgbuf,
		                         base::size(msgbuf), nullptr);
		if (len) {
			// Messages returned by system end with line breaks.
		    return base::CollapseWhitespaceASCII(msgbuf, true) +
		           base::StringPrintf(" (0x%lX)", error_code);
		}
		return base::StringPrintf("Error (0x%lX) while retrieving error. (0x%lX)",
		                        GetLastError(), error_code);
	}


	Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file,
	                                           int line,
	                                           LogSeverity severity,
	                                           SystemErrorCode err)
	    : err_(err),
	      log_message_(file, line, severity) {
	}

	Win32ErrorLogMessage::~Win32ErrorLogMessage() {
		stream() << ": " << SystemErrorCodeToString(err_);
		// We're about to crash (CHECK). Put |err_| on the stack (by placing it in a
		// field) and use Alias in hopes that it makes it into crash dumps.
		DWORD last_error = err_;
		base::debug::Alias(&last_error);
	}

	void CloseLogFile() {
  		CloseLogFileUnlocked();
	}

	void RawLog(int level, const char* message) {
		if (level >= g_min_log_level && message) {
			size_t bytes_written = 0;
			const size_t message_len = strlen(message);
			int rv;
			while (bytes_written < message_len) {
				rv = HANDLE_EINTR(
					write(STDERR_FILENO, message + bytes_written,
						message_len - bytes_written));
				if (rv < 0) {
					// Give up, nothing we can do now.
					break;
				}
				bytes_written += rv;
			}

			if (message_len > 0 && message[message_len - 1] != '\n') {
				do {
					rv = HANDLE_EINTR(write(STDERR_FILENO, "\n", 1));
					if (rv < 0) {
						// Give up, nothing we can do now.
						break;
					}
				} while (rv != 1);
			}
		}

		if (level == LOG_FATAL)
			base::debug::BreakDebugger();
	}

// This was defined at the beginning of this file.
	#undef write

	bool IsLoggingToFileEnabled() {
		return g_logging_destination & LOG_TO_FILE;
	}

	std::wstring GetLogFileFullPath() {
		if (g_log_file_name)
			return *g_log_file_name;
		return std::wstring();
	}

	BASE_EXPORT void LogErrorNotReached(const char* file, int line) {
		LogMessage(file, line, LOG_ERROR).stream() 
			<< "NOTREACHED() hit.";
	}

}  // namespace logging

std::ostream& std::operator<<(std::ostream& out, const wchar_t* wstr) {
	return out << (wstr ? base::WideToUTF8(wstr) : std::string());
}
