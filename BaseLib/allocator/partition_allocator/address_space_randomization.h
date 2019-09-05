// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "allocator/partition_allocator/page_allocator.h"
#include "base_export.h"
#include "build_config.h"

namespace base {

	// Calculates a random preferred mapping address. In calculating an address, we
	// balance good ASLR against not fragmenting the address space too badly.
	BASE_EXPORT void* GetRandomPageBase();

	namespace internal {

		constexpr uintptr_t AslrAddress(uintptr_t mask) {
			return mask & kPageAllocationGranularityBaseMask;
		}
		constexpr uintptr_t AslrMask(uintptr_t bits) {
			return AslrAddress((1ULL << bits) - 1ULL);
		}

		// Turn off formatting, because the thicket of nested ifdefs below is
		// incomprehensible without indentation. It is also incomprehensible with
		// indentation, but the only other option is a combinatorial explosion of
		// *_{win,linux,mac,foo}_{32,64}.h files.
		//
		// clang-format off

#if defined(ARCH_CPU_64_BITS)

#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)

		// We shouldn't allocate system pages at all for sanitizer builds. However,
		// we do, and if random hint addresses interfere with address ranges
		// hard-coded in those tools, bad things happen. This address range is
		// copied from TSAN source but works with all tools. See
		// https://crbug.com/539863.
		constexpr uintptr_t kASLRMask = AslrAddress(0x007fffffffffULL);
		constexpr uintptr_t kASLROffset = AslrAddress(0x7e8000000000ULL);

#else

  // Windows 8.10 and newer support the full 48 bit address range. Older
  // versions of Windows only support 44 bits. Since kASLROffset is non-zero
  // and may cause a carry, use 47 and 43 bit masks. See
  // http://www.alex-ionescu.com/?p=246
		constexpr uintptr_t kASLRMask = AslrMask(47);
		constexpr uintptr_t kASLRMaskBefore8_10 = AslrMask(43);
		// Try not to map pages into the range where Windows loads DLLs by default.
		constexpr uintptr_t kASLROffset = 0x80000000ULL;

#endif

#elif defined(ARCH_CPU_32_BITS)

  // This is a good range on 32-bit Windows and Android (the only platforms on
  // which we support 32-bitness). Allocates in the 0.5 - 1.5 GiB region. There
  // is no issue with carries here.
		constexpr uintptr_t kASLRMask = AslrMask(30);
		constexpr uintptr_t kASLROffset = AslrAddress(0x20000000ULL);

#else

#error Please tell us about your exotic hardware! Sounds interesting.

#endif  // defined(ARCH_CPU_32_BITS)

// clang-format on

	}  // namespace internal

}  // namespace base
