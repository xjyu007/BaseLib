
// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>

#include <atomic>

#include "base_export.h"
#include "macros.h"
#include "task/sequence_manager/enqueue_order.h"

namespace base {
	namespace sequence_manager {
		namespace internal {

			// EnqueueOrder can't be created from a raw number in non-test code.
			// EnqueueOrderGenerator is used to create it with strictly monotonic guarantee.
			class BASE_EXPORT EnqueueOrderGenerator {
			public:
				EnqueueOrderGenerator();
				~EnqueueOrderGenerator();

				// Can be called from any thread.
				EnqueueOrder GenerateNext() {
					return EnqueueOrder(std::atomic_fetch_add_explicit(
						&counter_, uint64_t(1), std::memory_order_relaxed));
				}

			private:
				std::atomic<uint64_t> counter_;
				DISALLOW_COPY_AND_ASSIGN(EnqueueOrderGenerator);
			};

		}  // namespace internal
	}  // namespace sequence_manager
}  // namespace base
