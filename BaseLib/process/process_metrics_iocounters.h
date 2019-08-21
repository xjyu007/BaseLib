#pragma once

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a separate file so that users of process metrics don't need to
// include windows.h unless they need IoCounters.

#include <stdint.h>

#include "process/process_metrics.h"
#include "build_config.h"

#include <windows.h>

namespace base {

	struct IoCounters : public IO_COUNTERS {};

}  // namespace base
