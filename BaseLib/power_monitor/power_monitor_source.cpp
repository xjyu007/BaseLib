// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power_monitor/power_monitor_source.h"

#include "power_monitor/power_monitor.h"
#include "build_config.h"

namespace base {

	PowerMonitorSource::PowerMonitorSource() = default;
	PowerMonitorSource::~PowerMonitorSource() = default;

	bool PowerMonitorSource::IsOnBatteryPower() {
		AutoLock auto_lock(battery_lock_);
		return on_battery_power_;
	}

	void PowerMonitorSource::ProcessPowerEvent(PowerEvent event_id) {
		if (!PowerMonitor::IsInitialized())
			return;

		auto source = PowerMonitor::Source();

		// Suppress duplicate notifications.  Some platforms may
		// send multiple notifications of the same event.
		switch (event_id) {
		case POWER_STATE_EVENT:
		{
			const auto new_on_battery_power = source->IsOnBatteryPowerImpl();
			auto changed = false;

			{
				AutoLock auto_lock(source->battery_lock_);
				if (source->on_battery_power_ != new_on_battery_power) {
					changed = true;
					source->on_battery_power_ = new_on_battery_power;
				}
			}

			if (changed)
				PowerMonitor::NotifyPowerStateChange(new_on_battery_power);
		}
		break;
		case RESUME_EVENT:
			if (source->suspended_) {
				source->suspended_ = false;
				PowerMonitor::NotifyResume();
			}
			break;
		case SUSPEND_EVENT:
			if (!source->suspended_) {
				source->suspended_ = true;
				PowerMonitor::NotifySuspend();
			}
			break;
		}
	}

	void PowerMonitorSource::SetInitialOnBatteryPowerState(bool on_battery_power) {
		// Must only be called before an initialized PowerMonitor exists, otherwise
		// the caller should have just used a normal
		// ProcessPowerEvent(POWER_STATE_EVENT) call.
		DCHECK(!PowerMonitor::Source());
		on_battery_power_ = on_battery_power;
	}

}  // namespace base
