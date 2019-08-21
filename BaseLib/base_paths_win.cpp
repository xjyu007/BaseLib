// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>
#include <KnownFolders.h>
#include <shlobj.h>

#include "base_paths.h"
#include "environment.h"
#include "files/file_path.h"
#include "path_service.h"
#include "strings/string_util.h"
#include "strings/utf_string_conversions.h"
#include "win/current_module.h"
#include "win/scoped_co_mem.h"
#include "win/windows_version.h"

using base::FilePath;

namespace base {

	bool PathProviderWin(int key, FilePath* result) {
		// We need to go compute the value. It would be nice to support paths with
		// names longer than MAX_PATH, but the system functions don't seem to be
		// designed for it either, with the exception of GetTempPath (but other
		// things will surely break if the temp path is too long, so we don't bother
		// handling it.
		wchar_t system_buffer[MAX_PATH];
		system_buffer[0] = 0;
		wchar_t* wsystem_buffer = as_writable_wcstr(system_buffer);

		FilePath cur;
		switch (key) {
		case base::FILE_EXE:
			if (GetModuleFileNameW(NULL, wsystem_buffer, MAX_PATH) == 0)
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::FILE_MODULE:
		{
			// the resource containing module is assumed to be the one that
			// this code lives in, whether that's a dll or exe
			if (GetModuleFileNameW(CURRENT_MODULE(), wsystem_buffer, MAX_PATH) == 0)
				return false;
			cur = FilePath(system_buffer);
			break;
		}
		case base::DIR_WINDOWS:
			if (0 == GetWindowsDirectoryW(wsystem_buffer, MAX_PATH))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_SYSTEM:
			if (0 == GetSystemDirectoryW(wsystem_buffer, MAX_PATH))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_PROGRAM_FILESX86:
			if (win::OSInfo::GetArchitecture() != win::OSInfo::X86_ARCHITECTURE) {
				if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILESX86, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
					return false;
				cur = FilePath(system_buffer);
				break;
			}
			// Fall through to base::DIR_PROGRAM_FILES if we're on an X86 machine.
			FALLTHROUGH;
		case base::DIR_PROGRAM_FILES:
			if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_PROGRAM_FILES6432:
#if !defined(_WIN64)
			if (base::win::OSInfo::GetInstance()->wow64_status() == base::win::OSInfo::WOW64_ENABLED) {
				std::unique_ptr<base::Environment> env(base::Environment::Create());
				std::string programfiles_w6432;
				// 32-bit process running in WOW64 sets ProgramW6432 environment
				// variable. See
				// https://msdn.microsoft.com/library/windows/desktop/aa384274.aspx.
				if (!env->GetVar("ProgramW6432", &programfiles_w6432))
					return false;
				// GetVar returns UTF8 - convert back to Wide.
				cur = FilePath(UTF8ToWide(programfiles_w6432));
				break;
			}
#endif
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_IE_INTERNET_CACHE:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_INTERNET_CACHE, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_COMMON_START_MENU:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_START_MENU:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_APP_DATA:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_COMMON_APP_DATA:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_LOCAL_APP_DATA:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer)))
				return false;
			cur = FilePath(system_buffer);
			break;
		case base::DIR_SOURCE_ROOT:
		{
			FilePath executableDir;
			// On Windows, unit tests execute two levels deep from the source root.
			// For example:  chrome/{Debug|Release}/ui_tests.exe
			PathService::Get(base::DIR_EXE, &executableDir);
			cur = executableDir.DirName().DirName();
			break;
		}
		case base::DIR_APP_SHORTCUTS:
		{
			if (win::GetVersion() < win::Version::WIN8)
				return false;

			base::win::ScopedCoMem<wchar_t> path_buf;
			if (FAILED(SHGetKnownFolderPath(FOLDERID_ApplicationShortcuts, 0, NULL, &path_buf)))
				return false;

			cur = FilePath(as_u16cstr(path_buf.get()));
			break;
		}
		case base::DIR_USER_DESKTOP:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer))) {
				return false;
			}
			cur = FilePath(system_buffer);
			break;
		case base::DIR_COMMON_DESKTOP:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer))) {
				return false;
			}
			cur = FilePath(system_buffer);
			break;
		case base::DIR_USER_QUICK_LAUNCH:
			if (!PathService::Get(base::DIR_APP_DATA, &cur))
				return false;
			// According to various sources, appending
			// "Microsoft\Internet Explorer\Quick Launch" to %appdata% is the only
			// reliable way to get the quick launch folder across all versions of
			// Windows.
			// http://stackoverflow.com/questions/76080/how-do-you-reliably-get-the-quick-
			// http://www.microsoft.com/technet/scriptcenter/resources/qanda/sept05/hey0901.mspx
			cur = cur.Append(FILE_PATH_LITERAL("Microsoft")).Append(FILE_PATH_LITERAL("Internet Explorer")).Append(FILE_PATH_LITERAL("Quick Launch"));
			break;
		case base::DIR_TASKBAR_PINS:
			if (!PathService::Get(base::DIR_USER_QUICK_LAUNCH, &cur))
				return false;
			cur = cur.Append(FILE_PATH_LITERAL("User Pinned")).Append(FILE_PATH_LITERAL("TaskBar"));
			break;
		case base::DIR_IMPLICIT_APP_SHORTCUTS:
			if (!PathService::Get(base::DIR_USER_QUICK_LAUNCH, &cur))
				return false;
			cur = cur.Append(FILE_PATH_LITERAL("User Pinned")).Append(FILE_PATH_LITERAL("ImplicitAppShortcuts"));
			break;
		case base::DIR_WINDOWS_FONTS:
			if (FAILED(SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, wsystem_buffer))) {
				return false;
			}
			cur = FilePath(system_buffer);
			break;
		default:
			return false;
		}

		*result = cur;
		return true;
	}

}  // namespace base