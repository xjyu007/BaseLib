// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "macros.h"
#include "power_monitor/power_monitor_source.h"
#include "build_config.h"

#include <windows.h>

namespace base {

	// A class used to monitor the power state change and notify the observers about
	// the change event.
	class BASE_EXPORT PowerMonitorDeviceSource : public PowerMonitorSource {
	public:
		PowerMonitorDeviceSource();
		~PowerMonitorDeviceSource() override;

	private:
		// Represents a message-only window for power message handling on Windows.
		// Only allow PowerMonitor to create it.
		class PowerMessageWindow {
		public:
			PowerMessageWindow();
			~PowerMessageWindow();

		private:
			static LRESULT CALLBACK WndProcThunk(HWND hwnd,
				UINT message,
				WPARAM wparam,
				LPARAM lparam);
			// Instance of the module containing the window procedure.
			HMODULE instance_;
			// A hidden message-only window.
			HWND message_hwnd_;
		};

		// Platform-specific method to check whether the system is currently
		// running on battery power.  Returns true if running on batteries,
		// false otherwise.
		bool IsOnBatteryPowerImpl() override;

		PowerMessageWindow power_message_window_;

		DISALLOW_COPY_AND_ASSIGN(PowerMonitorDeviceSource);
	};

}  // namespace base
