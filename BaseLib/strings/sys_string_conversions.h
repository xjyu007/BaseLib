#pragma once

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides system-dependent string type conversions for cases where it's
// necessary to not use ICU. Generally, you should not need this in Chrome,
// but it is used in some shared code. Dependencies should be minimal.

#include <cstdint>

#include <string>

#include "base_export.h"

namespace base {

	// Converts between wide and UTF-8 representations of a string. On error, the
	// result is system-dependent.
	BASE_EXPORT std::string SysWideToUTF8(const std::wstring& wide);
	BASE_EXPORT std::wstring SysUTF8ToWide(std::string_view utf8);

	// Converts between wide and the system multi-byte representations of a string.
	// DANGER: This will lose information and can change (on Windows, this can
	// change between reboots).
	BASE_EXPORT std::string SysWideToNativeMB(const std::wstring& wide);
	BASE_EXPORT std::wstring SysNativeMBToWide(std::string_view native_mb);

// Windows-specific ------------------------------------------------------------
	// Converts between 8-bit and wide strings, using the given code page. The
	// code page identifier is one accepted by the Windows function
	// MultiByteToWideChar().
	BASE_EXPORT std::wstring SysMultiByteToWide(std::string_view mb, uint32_t code_page);
	BASE_EXPORT std::string SysWideToMultiByte(const std::wstring& wide, uint32_t code_page);

}