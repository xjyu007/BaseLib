#pragma once

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base_export.h"
#include <string>

namespace base {

	// Returns true if the |string| passed in matches the |pattern|. The pattern
	// string can contain wildcards like * and ?.
	//
	// The backslash character (\) is an escape character for * and ?.
	// ? matches 0 or 1 character, while * matches 0 or more characters.
	BASE_EXPORT bool MatchPattern(std::string_view eval, std::string_view pattern);
	BASE_EXPORT bool MatchPattern(std::wstring_view eval, std::wstring_view pattern);

}  // namespace base

