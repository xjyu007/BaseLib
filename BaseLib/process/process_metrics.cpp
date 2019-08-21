// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "process/process_metrics.h"

#include <utility>

#include "logging.h"
#include "values.h"
#include "build_config.h"

namespace {
	int CalculateEventsPerSecond(uint64_t event_count, uint64_t* last_event_count, base::TimeTicks* last_calculated) {
		const auto time = base::TimeTicks::Now();

		if (*last_event_count == 0) {
			// First call, just set the last values.
			*last_calculated = time;
			*last_event_count = event_count;
			return 0;
		}

		const int64_t events_delta = event_count - *last_event_count;
		const auto time_delta = (time - *last_calculated).InMicroseconds();
		if (time_delta == 0) {
			NOTREACHED();
			return 0;
		}

		*last_calculated = time;
		*last_event_count = event_count;

		const auto events_delta_for_ms = events_delta * base::Time::kMicrosecondsPerSecond;
		// Round the result up by adding 1/2 (the second term resolves to 1/2 without
		// dropping down into floating point).
		return static_cast<int>((events_delta_for_ms + time_delta / 2) / time_delta);
	}

}  // namespace

namespace base {

	SystemMemoryInfoKB::SystemMemoryInfoKB() = default;

	SystemMemoryInfoKB::SystemMemoryInfoKB(const SystemMemoryInfoKB& other) = default;

	SystemMetrics::SystemMetrics() {
		committed_memory_ = 0;
	}

	SystemMetrics SystemMetrics::Sample() {
		SystemMetrics system_metrics;
		system_metrics.committed_memory_ = GetSystemCommitCharge();
		GetSystemPerformanceInfo(&system_metrics.performance_);
		return system_metrics;
	}

	std::unique_ptr<Value> SystemMetrics::ToValue() const {
		std::unique_ptr<DictionaryValue> res(new DictionaryValue());

		res->SetIntKey("committed_memory", static_cast<int>(committed_memory_));
		res->Set("perfinfo", performance_.ToValue());

		return std::move(res);
	}

	std::unique_ptr<ProcessMetrics> ProcessMetrics::CreateCurrentProcessMetrics() {
		return CreateProcessMetrics(base::GetCurrentProcessHandle());
	}

	double ProcessMetrics::GetPlatformIndependentCPUUsage() {
		const auto cumulative_cpu = GetCumulativeCPUUsage();
		const auto time = TimeTicks::Now();

		if (last_cumulative_cpu_.is_zero()) {
			// First call, just set the last values.
			last_cumulative_cpu_ = cumulative_cpu;
			last_cpu_time_ = time;
			return 0;
		}

		const auto system_time_delta = cumulative_cpu - last_cumulative_cpu_;
		const auto time_delta = time - last_cpu_time_;
		DCHECK(!time_delta.is_zero());
		if (time_delta.is_zero())
			return 0;

		last_cumulative_cpu_ = cumulative_cpu;
		last_cpu_time_ = time;

		return 100.0 * system_time_delta.InMicrosecondsF() /
			time_delta.InMicrosecondsF();
	}

	int ProcessMetrics::GetIdleWakeupsPerSecond() {
		NOTIMPLEMENTED();  // http://crbug.com/120488
		return 0;
	}

	uint64_t ProcessMetrics::GetDiskUsageBytesPerSecond() {
		const auto cumulative_disk_usage = GetCumulativeDiskUsageInBytes();
		return CalculateEventsPerSecond(cumulative_disk_usage,
			&last_cumulative_disk_usage_,
			&last_disk_usage_time_);
	}

}  // namespace base
