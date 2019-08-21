// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "win/scoped_winrt_initializer.h"

#include "test/gtest_util.h"
#include "win/com_init_util.h"
#include "win/scoped_com_initializer.h"
#include "win/windows_version.h"

namespace base::win
{

		TEST(ScopedWinrtInitializer, BasicFunctionality) {
			if (GetVersion() < Version::WIN8)
				return;

			AssertComApartmentType(ComApartmentType::NONE);
			{
				ScopedWinrtInitializer scoped_winrt_initializer;
				AssertComApartmentType(ComApartmentType::MTA);
			}
			AssertComApartmentType(ComApartmentType::NONE);
		}

		TEST(ScopedWinrtInitializer, ApartmentChangeCheck) {
			if (GetVersion() < Version::WIN8)
				return;

			ScopedCOMInitializer com_initializer;
			// ScopedCOMInitializer initialized an STA and the following should be a
			// failed request for an MTA.
			EXPECT_DCHECK_DEATH({ ScopedWinrtInitializer scoped_winrt_initializer; });
		}

		TEST(ScopedWinrtInitializer, VersionCheck) {
			if (GetVersion() >= Version::WIN8)
				return;

			// ScopedWinrtInitializer is unsupported on versions prior to Windows 8.
			EXPECT_DCHECK_DEATH({ ScopedWinrtInitializer scoped_winrt_initializer; });
		}
} // namespace base
