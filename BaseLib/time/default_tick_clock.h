// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "time/tick_clock.h"

namespace base {

	// DefaultTickClock is a TickClock implementation that uses TimeTicks::Now().
	// This is typically used by components that expose a SetTickClockForTesting().
	// Note: Overriding Time/TimeTicks altogether via
	// TaskEnvironment::TimeSource::MOCK_TIME is now the preferred of
	// overriding time in unit tests. As such, there shouldn't be many new use cases
	// for TickClock/DefaultTickClock anymore.
	class BASE_EXPORT DefaultTickClock : public TickClock {
	public:
		~DefaultTickClock() override;

		// Simply returns TimeTicks::Now().
		TimeTicks NowTicks() const override;

		// Returns a shared instance of DefaultTickClock. This is thread-safe.
		static const DefaultTickClock* GetInstance();
	};

}  // namespace base
