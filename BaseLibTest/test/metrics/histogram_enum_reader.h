// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <string>
#include <optional>

#include "metrics/histogram_base.h"

namespace base {

	using HistogramEnumEntryMap = std::map<HistogramBase::Sample, std::string>;

	// Find and read the enum with the given |enum_name| (with integer values) from
	// tools/metrics/histograms/enums.xml.
	//
	// Returns map { value => label } so that:
	//   <int value="9" label="enable-pinch-virtual-viewport"/>
	// becomes:
	//   { 9 => "enable-pinch-virtual-viewport" }
	// Returns empty base::nullopt on failure.
	std::optional<HistogramEnumEntryMap> ReadEnumFromEnumsXml(const std::string& enum_name);

}  // namespace base
