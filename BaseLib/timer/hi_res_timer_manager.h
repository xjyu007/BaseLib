// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "macros.h"
#include "power_monitor/power_observer.h"
#include "timer/timer.h"

namespace base {

	// Ensures that the Windows high resolution timer is only used
	// when not running on battery power.
	class BASE_EXPORT HighResolutionTimerManager : public base::PowerObserver {
	public:
		HighResolutionTimerManager();
		~HighResolutionTimerManager() override;

		// base::PowerObserver methods.
		void OnPowerStateChange(bool on_battery_power) override;
		void OnSuspend() override;
		void OnResume() override;

		// Returns true if the hi resolution clock could be used right now.
		bool hi_res_clock_available() const { return hi_res_clock_available_; }

	private:
		// Enable or disable the faster multimedia timer.
		void UseHiResClock(bool use);

		bool hi_res_clock_available_;

		// Timer for polling the high resolution timer usage.
		RepeatingTimer timer_;

		DISALLOW_COPY_AND_ASSIGN(HighResolutionTimerManager);
	};

}  // namespace base
