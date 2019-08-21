// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "task/thread_pool/sequence.h"

namespace base::internal
{

		class CanScheduleSequenceObserver {
		public:
			// Called when |sequence| can be scheduled. It is expected that
			// TaskTracker::RunNextTask() will be called with |sequence| as argument after
			// this is called.
			virtual void OnCanScheduleSequence(scoped_refptr<Sequence> sequence) = 0;

		protected:
			virtual ~CanScheduleSequenceObserver() = default;
		};
} // namespace base
