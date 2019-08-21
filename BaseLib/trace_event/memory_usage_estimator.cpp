// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "trace_event/memory_usage_estimator.h"

namespace base::trace_event
{

		template size_t EstimateMemoryUsage(const std::string&);
		template size_t EstimateMemoryUsage(const std::wstring&);
} // namespace base
