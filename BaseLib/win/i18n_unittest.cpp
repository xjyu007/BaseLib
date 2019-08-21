// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains unit tests for Windows internationalization funcs.

#include "gtest/gtest.h"

#include <stddef.h>

#include "win/i18n.h"
#include "win/windows_version.h"

namespace base {
	namespace win {
		namespace i18n {

			// Tests that at least one user preferred UI language can be obtained.
			TEST(I18NTest, GetUserPreferredUILanguageList) {
				std::vector<std::wstring> languages;
				EXPECT_TRUE(GetUserPreferredUILanguageList(&languages));
				EXPECT_FALSE(languages.empty());
				for (const auto& language : languages) {
					EXPECT_FALSE(language.empty());
				}
			}

			// Tests that at least one thread preferred UI language can be obtained.
			TEST(I18NTest, GetThreadPreferredUILanguageList) {
				std::vector<std::wstring> languages;
				EXPECT_TRUE(GetThreadPreferredUILanguageList(&languages));
				EXPECT_FALSE(languages.empty());
				for (const auto& language : languages) {
					EXPECT_FALSE(language.empty());
				}
			}

		}  // namespace i18n
	}  // namespace win
}  // namespace base
