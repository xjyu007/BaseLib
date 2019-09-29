// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include "unicode/unistr.h"
#include "unicode/uvernum.h"

#if U_ICU_VERSION_MAJOR_NUM >= 59
#include "unicode/char16ptr.h"
#endif

namespace base {
	namespace i18n {

		inline std::wstring UnicodeStringToString16(const icu::UnicodeString& unistr) {
#if U_ICU_VERSION_MAJOR_NUM >= 59
			return std::wstring(
				reinterpret_cast<const wchar_t*>(icu::toUCharPtr(unistr.getBuffer())),
				static_cast<std::wstring::size_type>(unistr.length()));
#else
			return std::wstring(unistr.getBuffer(),
				static_cast<size_t>(unistr.length()));
#endif
		}

	}  // namespace i18n
}  // namespace base
