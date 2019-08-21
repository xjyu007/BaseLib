// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "win/core_winrt_util.h"

#include "win/com_init_util.h"
#include "win/scoped_com_initializer.h"
#include "win/windows_version.h"

namespace base::win
{

		TEST(CoreWinrtUtilTest, PreloadFunctions) {
			if (GetVersion() < Version::WIN8)
				EXPECT_FALSE(ResolveCoreWinRTDelayload());
			else
				EXPECT_TRUE(ResolveCoreWinRTDelayload());
		}

		TEST(CoreWinrtUtilTest, RoInitializeAndUninitialize) {
			if (GetVersion() < Version::WIN8)
				return;

			ASSERT_TRUE(ResolveCoreWinRTDelayload());
			ASSERT_HRESULT_SUCCEEDED(base::win::RoInitialize(RO_INIT_MULTITHREADED));
			AssertComApartmentType(ComApartmentType::MTA);
			base::win::RoUninitialize();
			AssertComApartmentType(ComApartmentType::NONE);
		}
} // namespace base
