// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system/sys_info.h"

#include <algorithm>

#include "base_switches.h"
#include "bind.h"
#include "callback.h"
#include "command_line.h"
#include "feature_list.h"
#include "location.h"
#include "metrics/field_trial_params.h"
#include "no_destructor.h"
#include "system/sys_info_internal.h"
#include "task/post_task.h"
#include "task/task_traits.h"
#include "task_runner_util.h"
#include "time/time.h"

namespace base {
	namespace {

		// Feature used to control the heuristics used to categorize a device as low
		// end.
		const Feature kLowEndDeviceDetectionFeature{
			"LowEndDeviceDetection", FEATURE_DISABLED_BY_DEFAULT };

		const int kLowMemoryDeviceThresholdMBDefault = 512;

		int GetLowMemoryDeviceThresholdMB() {
			static constexpr FeatureParam<int> kLowEndDeviceMemoryThresholdMB{
				&kLowEndDeviceDetectionFeature, "LowEndDeviceMemoryThresholdMB",
				kLowMemoryDeviceThresholdMBDefault };
			// If the feature is disabled then |Get| will return the default value.
			return kLowEndDeviceMemoryThresholdMB.Get();
		}
	}  // namespace

	// static
	int64_t SysInfo::AmountOfPhysicalMemory() {
		if (CommandLine::ForCurrentProcess()->HasSwitch(
				switches::kEnableLowEndDeviceMode)) {
			return GetLowMemoryDeviceThresholdMB() * 1024 * 1024;
		}

		return AmountOfPhysicalMemoryImpl();
	}

	// static
	int64_t SysInfo::AmountOfAvailablePhysicalMemory() {
		if (CommandLine::ForCurrentProcess()->HasSwitch(
				switches::kEnableLowEndDeviceMode)) {
			// Estimate the available memory by subtracting our memory used estimate
			// from the fake |GetLowMemoryDeviceThresholdMB()| limit.
			const auto memory_used = 
				static_cast<size_t>(AmountOfPhysicalMemoryImpl() - AmountOfAvailablePhysicalMemoryImpl());
			const size_t memory_limit = GetLowMemoryDeviceThresholdMB() * 1024 * 1024;
			// std::min ensures no underflow, as |memory_used| can be > |memory_limit|.
			return memory_limit - std::min(memory_used, memory_limit);
		}

		return AmountOfAvailablePhysicalMemoryImpl();
	}

	bool SysInfo::IsLowEndDevice() {
		if (CommandLine::ForCurrentProcess()->HasSwitch(
				switches::kEnableLowEndDeviceMode)) {
			return true;
		}

		return IsLowEndDeviceImpl();
	}

	bool DetectLowEndDevice() {
		const auto command_line = CommandLine::ForCurrentProcess();
		if (command_line->HasSwitch(switches::kEnableLowEndDeviceMode))
			return true;
		if (command_line->HasSwitch(switches::kDisableLowEndDeviceMode))
			return false;

		const auto ram_size_mb = SysInfo::AmountOfPhysicalMemoryMB();
		return (ram_size_mb > 0 && ram_size_mb <= GetLowMemoryDeviceThresholdMB());
	}

	// static
	bool SysInfo::IsLowEndDeviceImpl() {
		static NoDestructor<
			internal::LazySysInfoValue<bool, DetectLowEndDevice>>
			instance;
		return instance->value();
	}

	std::string SysInfo::HardwareModelName() {
		return std::string();
	}

	void SysInfo::GetHardwareInfo(OnceCallback<void(HardwareInfo)> callback) {
		PostTaskAndReplyWithResult(
		CreateCOMSTATaskRunner({ThreadPool()}).get(), FROM_HERE,
		BindOnce(&GetHardwareInfoSync), std::move(callback));
	}

	// static
	TimeDelta SysInfo::Uptime() {
		// This code relies on an implementation detail of TimeTicks::Now() - that
		// its return value happens to coincide with the system uptime value in
		// microseconds, on Win/Mac/iOS/Linux/ChromeOS and Android.
		const auto uptime_in_microseconds = TimeTicks::Now().ToInternalValue();
		return TimeDelta::FromMicroseconds(uptime_in_microseconds);
	}

}  // namespace base
