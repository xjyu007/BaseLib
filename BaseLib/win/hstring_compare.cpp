// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win/hstring_compare.h"

#include <winstring.h>

#include "native_library.h"
#include "win/windows_version.h"

namespace base::win {

	HRESULT HStringCompare(HSTRING string1, HSTRING string2, INT32* result) {
		using CompareStringFunc = decltype(&::WindowsCompareStringOrdinal);

		static const auto compare_string_func = []() -> CompareStringFunc {
			if (GetVersion() < Version::WIN8)
				return nullptr;

			NativeLibraryLoadError load_error;
			const auto combase_module =
				PinSystemLibrary(FILE_PATH_LITERAL("combase.dll"), &load_error);
			if (load_error.code)
				return nullptr;

			return reinterpret_cast<CompareStringFunc>(
				GetFunctionPointerFromNativeLibrary(combase_module,
					"WindowsCompareStringOrdinal"));
		}();

		if (!compare_string_func)
			return E_FAIL;

		return compare_string_func(string1, string2, result);
	}
} // namespace base
