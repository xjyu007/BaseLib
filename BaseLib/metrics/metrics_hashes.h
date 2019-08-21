#pragma once

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "base_export.h"
#include <string>

namespace base {

	// Computes a uint64_t hash of a given string based on its MD5 hash. Suitable
	// for metric names.
	BASE_EXPORT uint64_t HashMetricName(std::string_view name);

}  // namespace metrics

