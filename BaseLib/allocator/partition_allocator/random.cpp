// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "allocator/partition_allocator/random.h"

#include "allocator/partition_allocator/spin_lock.h"
#include "no_destructor.h"
#include "rand_util.h"

namespace base {

	// This is the same PRNG as used by tcmalloc for mapping address randomness;
	// see http://burtleburtle.net/bob/rand/smallprng.html
	struct RandomContext {
		subtle::SpinLock lock;
		bool initialized = false;
		uint32_t a = 0;
		uint32_t b = 0;
		uint32_t c = 0;
		uint32_t d = 0;
	};

	RandomContext* GetRandomContext() {
		static NoDestructor<RandomContext> s_RandomContext;
		return s_RandomContext.get();
	}

	uint32_t RandomValue(RandomContext* x) {
		subtle::SpinLock::Guard guard(x->lock);
		if (UNLIKELY(!x->initialized)) {
			const auto r1 = RandUint64();
			const auto r2 = RandUint64();
			x->a = static_cast<uint32_t>(r1);
			x->b = static_cast<uint32_t>(r1 >> 32);
			x->c = static_cast<uint32_t>(r2);
			x->d = static_cast<uint32_t>(r2 >> 32);
			x->initialized = true;
		}

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
		const auto e = x->a - rot(x->b, 27);
		x->a = x->b ^ rot(x->c, 17);
		x->b = x->c + x->d;
		x->c = x->d + e;
		x->d = e + x->a;
		return x->d;
#undef rot
	}

	void SetRandomPageBaseSeed(int64_t seed) {
		auto x = GetRandomContext();
		subtle::SpinLock::Guard guard(x->lock);
		x->a = x->b = static_cast<uint32_t>(seed);
		x->c = x->d = static_cast<uint32_t>(seed >> 32);
		x->initialized = true;
	}

}  // namespace base
