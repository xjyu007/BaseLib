#pragma once

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <string>
#include "base_export.h"

namespace base {

	// These convert between UTF-8, -16, and -32 strings. They are potentially slow,
	// so avoid unnecessary conversions. The low-level versions return a boolean
	// indicating whether the conversion was 100% valid. In this case, it will still
	// do the best it can and put the result in the output buffer. The versions that
	// return strings ignore this error and just return the best conversion
	// possible.
	BASE_EXPORT bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output);
	BASE_EXPORT std::string WideToUTF8(std::wstring_view wide);
	BASE_EXPORT bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output);
	BASE_EXPORT std::wstring UTF8ToWide(std::string_view utf8);

	// This converts an ASCII string, typically a hardcoded constant, to a UTF16
	// string.
	BASE_EXPORT std::wstring ASCIIToUTF16(std::string_view ascii);

	// Converts to 7-bit ASCII by truncating. The result must be known to be ASCII
	// beforehand.
	BASE_EXPORT std::string UTF16ToASCII(std::wstring_view utf16);

}  // namespace base

