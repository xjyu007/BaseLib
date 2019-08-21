#pragma once

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdarg>   // va_list

#include <string>

#include "base_export.h"
#include "compiler_specific.h"

namespace base {
	// Return a C++ string given printf-like input.
	BASE_EXPORT std::string StringPrintf(const char* format, ...) PRINTF_FORMAT(1, 2) WARN_UNUSED_RESULT;
	BASE_EXPORT std::wstring StringPrintf(const wchar_t* format, ...) WPRINTF_FORMAT(1, 2) WARN_UNUSED_RESULT;

	// Return a C++ string given vprintf-like input.
	BASE_EXPORT std::string StringPrintV(const char* format, va_list ap) PRINTF_FORMAT(1, 0) WARN_UNUSED_RESULT;

	// Store result into a supplied string and return it.
	BASE_EXPORT const std::string& SStringPrintf(std::string* dst, const char* format, ...) PRINTF_FORMAT(2, 3);
	BASE_EXPORT const std::wstring& SStringPrintf(std::wstring* dst, const wchar_t* format, ...) WPRINTF_FORMAT(2, 3);

	// Append result to a supplied string.
	BASE_EXPORT void StringAppendF(std::string* dst, const char* format, ...) PRINTF_FORMAT(2, 3);
	BASE_EXPORT void StringAppendF(std::wstring* dst, const wchar_t* format, ...) WPRINTF_FORMAT(2, 3);

	// Lower-level routine that takes a va_list and appends to a specified
	// string.  All other routines are just convenience wrappers around it.
	BASE_EXPORT void StringAppendV(std::string* dst, const char* format, va_list ap) PRINTF_FORMAT(2, 0);
	BASE_EXPORT void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap) WPRINTF_FORMAT(2, 0);

}  // namespace base
