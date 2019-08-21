#pragma once

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base_export.h"
#include "macros.h"
#include "time/time.h"

namespace base {

	// A simple wrapper around TimeTicks::Now().
	class BASE_EXPORT ElapsedTimer {
	public:
		ElapsedTimer();
		ElapsedTimer(ElapsedTimer&& other) noexcept;

		void operator=(ElapsedTimer&& other) noexcept;

		// Returns the time elapsed since object construction.
		[[nodiscard]] TimeDelta Elapsed() const;

		// Returns the timestamp of the creation of this timer.
		[[nodiscard]] TimeTicks Begin() const { return begin_; }

	private:
		TimeTicks begin_;

		DISALLOW_COPY_AND_ASSIGN(ElapsedTimer);
	};

	// A simple wrapper around ThreadTicks::Now().
	class BASE_EXPORT ElapsedThreadTimer {
	public:
		ElapsedThreadTimer();

		// Returns the ThreadTicks time elapsed since object construction.
		// Only valid if |is_supported()| returns true, otherwise returns TimeDelta().
		[[nodiscard]] TimeDelta Elapsed() const;

		[[nodiscard]] bool is_supported() const { return is_supported_; }

	private:
		const bool is_supported_;
		const ThreadTicks begin_;

		DISALLOW_COPY_AND_ASSIGN(ElapsedThreadTimer);
	};

	// Whenever there's a ScopedMockElapsedTimersForTest in scope,
	// Elapsed(Thread)Timers will always return kMockElapsedTime from Elapsed().
	// This is useful, for example, in unit tests that verify that their impl
	// records timing histograms. It enables such tests to observe reliable timings.
	class BASE_EXPORT ScopedMockElapsedTimersForTest {
	public:
		static constexpr TimeDelta kMockElapsedTime =
			TimeDelta::FromMilliseconds(1337);

		// ScopedMockElapsedTimersForTest is not thread-safe (it must be instantiated
		// in a test before other threads begin using ElapsedTimers; and it must
		// conversely outlive any usage of ElapsedTimer in that test).
		ScopedMockElapsedTimersForTest();
		~ScopedMockElapsedTimersForTest();

	private:
		DISALLOW_COPY_AND_ASSIGN(ScopedMockElapsedTimersForTest);
	};

}  // namespace base
