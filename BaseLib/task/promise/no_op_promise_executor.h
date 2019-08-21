// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "task/promise/abstract_promise.h"

namespace base {
	namespace internal {

		// An Executor that doesn't do anything.
		class BASE_EXPORT NoOpPromiseExecutor {
		public:
			NoOpPromiseExecutor(bool can_resolve, bool can_reject);

			~NoOpPromiseExecutor();

			static scoped_refptr<internal::AbstractPromise> Create(
				const Location& from_here,
				bool can_resolve,
				bool can_reject,
				RejectPolicy reject_policy);

			[[nodiscard]] PromiseExecutor::PrerequisitePolicy GetPrerequisitePolicy() const;
			[[nodiscard]] static bool IsCancelled();

#if DCHECK_IS_ON()
			[[nodiscard]] static PromiseExecutor::ArgumentPassingType ResolveArgumentPassingType();
			[[nodiscard]] static PromiseExecutor::ArgumentPassingType RejectArgumentPassingType();
			[[nodiscard]] bool CanResolve() const;
			[[nodiscard]] bool CanReject() const;
#endif
			static void Execute(AbstractPromise* promise);

		private:
#if DCHECK_IS_ON()
			bool can_resolve_;
			bool can_reject_;
#endif
		};

	}  // namespace internal
}  // namespace base
