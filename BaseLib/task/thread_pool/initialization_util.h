// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"

namespace base {

	// Computes a value that may be used as the maximum number of threads in a
	// ThreadGroup. Developers may use other methods to choose this maximum.
	BASE_EXPORT int RecommendedMaxNumberOfThreadsInThreadGroup(
		int min,
		int max,
		double cores_multiplier,
		int offset);

}  // namespace base
