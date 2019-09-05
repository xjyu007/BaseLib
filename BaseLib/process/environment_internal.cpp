// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "process/environment_internal.h"
#include "logging.h"

namespace base::internal {

	namespace {

		// Parses a null-terminated input string of an environment block. The key is
		// placed into the given string, and the total length of the line, including
		// the terminating null, is returned.
		size_t ParseEnvLine(const NativeEnvironmentString::value_type * input,
			NativeEnvironmentString * key) {
			// Skip to the equals or end of the string, this is the key.
			size_t cur = 0;
			while (input[cur] && input[cur] != '=')
				cur++;
			*key = NativeEnvironmentString(&input[0], cur);

			// Now just skip to the end of the string.
			while (input[cur])
				cur++;
			return cur + 1;
		}

	}  // namespace

	NativeEnvironmentString AlterEnvironment(const wchar_t* env, const EnvironmentMap& changes) {
		NativeEnvironmentString result;

		// First build up all of the unchanged environment strings.
		const wchar_t* ptr = env;
		while (*ptr) {
			std::wstring key;
			size_t line_length = ParseEnvLine(ptr, &key);

			// Keep only values not specified in the change vector.
			if (changes.find(key) == changes.end()) {
				result.append(ptr, line_length);
			}
			ptr += line_length;
		}

		// Now append all modified and new values.
		for (const auto& i : changes) {
			// Windows environment blocks cannot handle keys or values with NULs.
			CHECK_EQ(std::wstring::npos, i.first.find(static_cast<wchar_t>(0)));
			CHECK_EQ(std::wstring::npos, i.second.find(static_cast<wchar_t>(0)));
			if (!i.second.empty()) {
				result += i.first;
				result.push_back('=');
				result += i.second;
				result.push_back('\0');
			}
		}

		// Add the terminating NUL.
		result.push_back('\0');
		return result;
	}
} // namespace base
