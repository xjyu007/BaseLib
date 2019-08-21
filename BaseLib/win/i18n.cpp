// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win/i18n.h"

#include <Windows.h>

#include "logging.h"
#include "strings/string_util.h"
#include "strings/string_split.h"

namespace {

	typedef decltype(::GetSystemPreferredUILanguages)* GetPreferredUILanguages_Fn;

	bool GetPreferredUILanguageList(GetPreferredUILanguages_Fn function,
									ULONG flags,
									std::vector<std::wstring>* languages) {
		DCHECK_EQ((flags & (MUI_LANGUAGE_ID | MUI_LANGUAGE_NAME)), 0U);
		const ULONG call_flags = flags | MUI_LANGUAGE_NAME;
		ULONG language_count = 0;
		ULONG buffer_length = 0;
		if (!function(call_flags, &language_count, nullptr, &buffer_length) ||
			!buffer_length) {
			DPCHECK(!buffer_length)	<< "Failed getting size of preferred UI languages.";
			return false;
		}

		std::wstring buffer(buffer_length, '\0');
		if (!function(call_flags, &language_count, base::as_writable_wcstr(buffer),
			&buffer_length) ||
			!language_count) {
			DPCHECK(!language_count) << "Failed getting preferred UI languages.";
			return false;
		}

		// Split string on NUL characters.
		*languages =
			base::SplitString(buffer, std::wstring(1, '\0'), base::KEEP_WHITESPACE,
				base::SPLIT_WANT_NONEMPTY);
		DCHECK_EQ(languages->size(), language_count);
		return true;
	}

}  // namespace

namespace base::win::i18n {

	bool GetUserPreferredUILanguageList(std::vector<std::wstring>* languages) {
		DCHECK(languages);
		return GetPreferredUILanguageList(::GetUserPreferredUILanguages, 0,
			languages);
	}

	bool GetThreadPreferredUILanguageList(std::vector<std::wstring>* languages) {
		DCHECK(languages);
		return GetPreferredUILanguageList(
			::GetThreadPreferredUILanguages,
			MUI_MERGE_SYSTEM_FALLBACK | MUI_MERGE_USER_FALLBACK, languages);
	}
} // namespace base::win::i18n
