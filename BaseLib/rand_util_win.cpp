// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rand_util.h"

#include <Windows.h>
#include <cstdint>

// #define needed to link in RtlGenRandom(), a.k.a. SystemFunction036.  See the
// "Community Additions" comment on MSDN here:
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036

#include <algorithm>
#include <limits>

#include "logging.h"

#undef min
#undef max

namespace base {

	void RandBytes(void* output, size_t output_length) {
		auto output_ptr = static_cast<char*>(output);
		while (output_length > 0) {
			const auto output_bytes_this_pass = static_cast<ULONG>(std::min(
				output_length, static_cast<size_t>(std::numeric_limits<ULONG>::max())));
			const auto success = RtlGenRandom(output_ptr, output_bytes_this_pass) != FALSE;
			CHECK(success);
			output_length -= output_bytes_this_pass;
			output_ptr += output_bytes_this_pass;
		}
	}

}  // namespace base
