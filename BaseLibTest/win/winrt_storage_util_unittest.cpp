// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "win/winrt_storage_util.h"

#include <string.h>
#include <wrl/client.h>

#include "strings/string_util.h"
#include "win/core_winrt_util.h"
#include "win/scoped_com_initializer.h"
#include "win/scoped_hstring.h"

namespace base::win
{

		TEST(WinrtStorageUtilTest, CreateBufferFromData) {
			ScopedCOMInitializer com_initializer(ScopedCOMInitializer::kMTA);

			if (!ResolveCoreWinRTDelayload() ||
				!ScopedHString::ResolveCoreWinRTStringDelayload()) {
				return;
			}

			const std::vector<uint8_t> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
			Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
			ASSERT_HRESULT_SUCCEEDED(
				CreateIBufferFromData(data.data(), data.size(), &buffer));

			uint8_t* p_buffer_data;
			uint32_t length;
			ASSERT_HRESULT_SUCCEEDED(
				GetPointerToBufferData(buffer.Get(), &p_buffer_data, &length));

			ASSERT_EQ(data.size(), length);
			EXPECT_EQ(0, memcmp(p_buffer_data, data.data(), data.size()));
		}
} // namespace base
