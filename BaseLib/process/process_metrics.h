#pragma once

// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains routines for gathering resource statistics for processes
// running on the system.

#include <cstdint>

#include <memory>

#include "base_export.h"
//#include "gtest_prod_util.h"
#include "macros.h"
#include "process/process_handle.h"
#include "time/time.h"
#include "values.h"
#include "build_config.h"

#include "win/scoped_handle.h"

namespace base {

	// Full declaration is in process_metrics_iocounters.h.
	struct IoCounters;

	// Convert a POSIX timeval to microseconds.
	BASE_EXPORT int64_t TimeValToMicroseconds(const struct timeval& tv);

	// Provides performance metrics for a specified process (CPU usage and IO
	// counters). Use CreateCurrentProcessMetrics() to get an instance for the
	// current process, or CreateProcessMetrics() to get an instance for an
	// arbitrary process. Then, access the information with the different get
	// methods.
	//
	// This class exposes a few platform-specific APIs for parsing memory usage, but
	// these are not intended to generalize to other platforms, since the memory
	// models differ substantially.
	//
	// To obtain consistent memory metrics, use the memory_instrumentation service.
	//
	// For further documentation on memory, see
	// https://chromium.googlesource.com/chromium/src/+/HEAD/docs/README.md
	class BASE_EXPORT ProcessMetrics {
	public:
		~ProcessMetrics();

		// Creates a ProcessMetrics for the specified process.
		static std::unique_ptr<ProcessMetrics> CreateProcessMetrics(
			ProcessHandle process);

		// Creates a ProcessMetrics for the current process. This a cross-platform
		// convenience wrapper for CreateProcessMetrics().
		static std::unique_ptr<ProcessMetrics> CreateCurrentProcessMetrics();

		// Returns the percentage of time spent executing, across all threads of the
		// process, in the interval since the last time the method was called. Since
		// this considers the total execution time across all threads in a process,
		// the result can easily exceed 100% in multi-thread processes running on
		// multi-core systems. In general the result is therefore a value in the
		// range 0% to SysInfo::NumberOfProcessors() * 100%.
		//
		// To obtain the percentage of total available CPU resources consumed by this
		// process over the interval, the caller must divide by NumberOfProcessors().
		//
		// Since this API measures usage over an interval, it will return zero on the
		// first call, and an actual value only on the second and subsequent calls.
		double GetPlatformIndependentCPUUsage();

		// Returns the cumulative CPU usage across all threads of the process since
		// process start. In case of multi-core processors, a process can consume CPU
		// at a rate higher than wall-clock time, e.g. two cores at full utilization
		// will result in a time delta of 2 seconds/per 1 wall-clock second.
		TimeDelta GetCumulativeCPUUsage();

		// Returns the number of average idle cpu wakeups per second since the last
		// call.
		int GetIdleWakeupsPerSecond();

		// Retrieves accounting information for all I/O operations performed by the
		// process.
		// If IO information is retrieved successfully, the function returns true
		// and fills in the IO_COUNTERS passed in. The function returns false
		// otherwise.
		bool GetIOCounters(IoCounters* io_counters) const;

		// Returns the number of bytes transferred to/from disk per second, across all
		// threads of the process, in the interval since the last time the method was
		// called.
		//
		// Since this API measures usage over an interval, it will return zero on the
		// first call, and an actual value only on the second and subsequent calls.
		uint64_t GetDiskUsageBytesPerSecond();

		// Returns the cumulative disk usage in bytes across all threads of the
		// process since process start.
		uint64_t GetCumulativeDiskUsageInBytes();

		// Returns total memory usage of malloc.
		size_t GetMallocUsage();

	private:
		explicit ProcessMetrics(ProcessHandle process);

		win::ScopedHandle process_;

		// Used to store the previous times and CPU usage counts so we can
		// compute the CPU usage between calls.
		TimeTicks last_cpu_time_;
		TimeDelta last_cumulative_cpu_;

		// Used to store the previous times and disk usage counts so we can
		// compute the disk usage between calls.
		TimeTicks last_disk_usage_time_;
		// Number of bytes transferred to/from disk in bytes.
		uint64_t last_cumulative_disk_usage_ = 0;

		DISALLOW_COPY_AND_ASSIGN(ProcessMetrics);
	};

	// Returns the memory committed by the system in KBytes.
	// Returns 0 if it can't compute the commit charge.
	BASE_EXPORT size_t GetSystemCommitCharge();

	// Returns the number of bytes in a memory page. Do not use this to compute
	// the number of pages in a block of memory for calling mincore(). On some
	// platforms, e.g. iOS, mincore() uses a different page size from what is
	// returned by GetPageSize().
	BASE_EXPORT size_t GetPageSize();

	// Returns the maximum number of file descriptors that can be open by a process
	// at once. If the number is unavailable, a conservative best guess is returned.
	BASE_EXPORT size_t GetMaxFds();

	// Returns the maximum number of handles that can be open at once per process.
	BASE_EXPORT size_t GetHandleLimit();

	// Data about system-wide memory consumption. Values are in KB. Available on
	// Windows, Mac, Linux, Android and Chrome OS.
	//
	// Total memory are available on all platforms that implement
	// GetSystemMemoryInfo(). Total/free swap memory are available on all platforms
	// except on Mac. Buffers/cached/active_anon/inactive_anon/active_file/
	// inactive_file/dirty/reclaimable/pswpin/pswpout/pgmajfault are available on
	// Linux/Android/Chrome OS. Shmem/slab/gem_objects/gem_size are Chrome OS only.
	// Speculative/file_backed/purgeable are Mac and iOS only.
	// Free is absent on Windows (see "avail_phys" below).
	struct BASE_EXPORT SystemMemoryInfoKB {
		SystemMemoryInfoKB();
		SystemMemoryInfoKB(const SystemMemoryInfoKB& other);

		// Serializes the platform specific fields to value.
		[[nodiscard]] std::unique_ptr<DictionaryValue> ToValue() const;

		int total = 0;

		// "This is the amount of physical memory that can be immediately reused
		// without having to write its contents to disk first. It is the sum of the
		// size of the standby, free, and zero lists." (MSDN).
		// Standby: not modified pages of physical ram (file-backed memory) that are
		// not actively being used.
		int avail_phys = 0;

		int swap_total = 0;
		int swap_free = 0;
	};

	// On Linux/Android/Chrome OS, system-wide memory consumption data is parsed
	// from /proc/meminfo and /proc/vmstat. On Windows/Mac, it is obtained using
	// system API calls.
	//
	// Fills in the provided |meminfo| structure. Returns true on success.
	// Exposed for memory debugging widget.
	BASE_EXPORT bool GetSystemMemoryInfo(SystemMemoryInfoKB* meminfo);

	struct BASE_EXPORT SystemPerformanceInfo {
		SystemPerformanceInfo();
		SystemPerformanceInfo(const SystemPerformanceInfo& other);

		// Serializes the platform specific fields to value.
		std::unique_ptr<Value> ToValue() const;

		// Total idle time of all processes in the system (units of 100 ns).
		uint64_t idle_time = 0;
		// Number of bytes read.
		uint64_t read_transfer_count = 0;
		// Number of bytes written.
		uint64_t write_transfer_count = 0;
		// Number of bytes transferred (e.g. DeviceIoControlFile)
		uint64_t other_transfer_count = 0;
		// The amount of read operations.
		uint64_t read_operation_count = 0;
		// The amount of write operations.
		uint64_t write_operation_count = 0;
		// The amount of other operations.
		uint64_t other_operation_count = 0;
		// The number of pages written to the system's pagefiles.
		uint64_t pagefile_pages_written = 0;
		// The number of write operations performed on the system's pagefiles.
		uint64_t pagefile_pages_write_ios = 0;
		// The number of pages of physical memory available to processes running on
		// the system.
		uint64_t available_pages = 0;
		// The number of pages read from disk to resolve page faults.
		uint64_t pages_read = 0;
		// The number of read operations initiated to resolve page faults.
		uint64_t page_read_ios = 0;
	};

	// Retrieves performance counters from the operating system.
	// Fills in the provided |info| structure. Returns true on success.
	BASE_EXPORT bool GetSystemPerformanceInfo(SystemPerformanceInfo* info);

	// Collects and holds performance metrics for system memory and disk.
	// Provides functionality to retrieve the data on various platforms and
	// to serialize the stored data.
	class BASE_EXPORT SystemMetrics {
	public:
		SystemMetrics();

		static SystemMetrics Sample();

		// Serializes the system metrics to value.
		std::unique_ptr<Value> ToValue() const;

	private:
		//FRIEND_TEST_ALL_PREFIXES(SystemMetricsTest, SystemMetrics);

		size_t committed_memory_;
		SystemPerformanceInfo performance_;
	};

}  // namespace base
