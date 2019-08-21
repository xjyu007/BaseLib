// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "synchronization/waitable_event.h"

#include <Windows.h>

#include <algorithm>
#include <utility>
#include <optional>

#include "debug_/activity_tracker.h"
#include "logging.h"
#include "numerics/safe_conversions.h"
#include "threading/scoped_blocking_call.h"
#include "threading/thread_restrictions.h"
#include "time/time.h"

namespace base {

	WaitableEvent::WaitableEvent(ResetPolicy reset_policy,
		InitialState initial_state)
		: handle_(CreateEvent(nullptr,
			reset_policy == ResetPolicy::MANUAL,
			initial_state == InitialState::SIGNALED,
			nullptr)) {
		// We're probably going to crash anyways if this is ever NULL, so we might as
		// well make our stack reports more informative by crashing here.
		CHECK(handle_.IsValid());
	}

	WaitableEvent::WaitableEvent(win::ScopedHandle handle)
		: handle_(std::move(handle)) {
		CHECK(handle_.IsValid()) << "Tried to create WaitableEvent from NULL handle";
	}

	WaitableEvent::~WaitableEvent() = default;

	void WaitableEvent::Reset() const {
		ResetEvent(handle_.Get());
	}

	void WaitableEvent::Signal() const {
		SetEvent(handle_.Get());
	}

	bool WaitableEvent::IsSignaled() const {
		const auto result = WaitForSingleObject(handle_.Get(), 0);
		DCHECK(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
			<< "Unexpected WaitForSingleObject result " << result;
		return result == WAIT_OBJECT_0;
	}

	void WaitableEvent::Wait() {
		// Record the event that this thread is blocking upon (for hang diagnosis) and
		// consider it blocked for scheduling purposes. Ignore this for non-blocking
		// WaitableEvents.
		std::optional<debug::ScopedEventWaitActivity> event_activity;
		std::optional<internal::ScopedBlockingCallWithBaseSyncPrimitives>
			scoped_blocking_call;
		if (waiting_is_blocking_) {
			event_activity.emplace(this);
			scoped_blocking_call.emplace(BlockingType::MAY_BLOCK);
		}

		const auto result = WaitForSingleObject(handle_.Get(), INFINITE);
		// It is most unexpected that this should ever fail.  Help consumers learn
		// about it if it should ever fail.
		DPCHECK(result != WAIT_FAILED);
		DCHECK_EQ(WAIT_OBJECT_0, result);
	}

	bool WaitableEvent::TimedWait(const TimeDelta& wait_delta) {
		if (wait_delta <= TimeDelta())
			return IsSignaled();

		// Record the event that this thread is blocking upon (for hang diagnosis) and
		// consider it blocked for scheduling purposes. Ignore this for non-blocking
		// WaitableEvents.
		std::optional<debug::ScopedEventWaitActivity> event_activity;
		std::optional<internal::ScopedBlockingCallWithBaseSyncPrimitives>
			scoped_blocking_call;
		if (waiting_is_blocking_) {
			event_activity.emplace(this);
			scoped_blocking_call.emplace(BlockingType::MAY_BLOCK);
		}

		// TimeTicks takes care of overflow but we special case is_max() nonetheless
		// to avoid invoking Now() unnecessarily.
		// WaitForSingleObject(handle_.Get(), INFINITE) doesn't spuriously wakeup so
		// we don't need to worry about is_max() for the increment phase of the loop.
		const auto end_time =
			wait_delta.is_max() ? TimeTicks::Max() : TimeTicks::Now() + wait_delta;
		for (auto remaining = wait_delta; remaining > TimeDelta();
			remaining = end_time - TimeTicks::Now()) {
			// Truncate the timeout to milliseconds, rounded up to avoid spinning
			// (either by returning too early or because a < 1ms timeout on Windows
			// tends to return immediately).
			const auto timeout_ms =
				remaining.is_max()
				? INFINITE
				: saturated_cast<DWORD>(remaining.InMillisecondsRoundedUp());
			const auto result = WaitForSingleObject(handle_.Get(), timeout_ms);
			DCHECK(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
				<< "Unexpected WaitForSingleObject result " << result;
			switch (result) {
			case WAIT_OBJECT_0:
				return true;
			case WAIT_TIMEOUT:
				// TimedWait can time out earlier than the specified |timeout| on
				// Windows. To make this consistent with the posix implementation we
				// should guarantee that TimedWait doesn't return earlier than the
				// specified |max_time| and wait again for the remaining time.
				continue;
			default: ;
			}
		}
		return false;
	}

	// static
	size_t WaitableEvent::WaitMany(WaitableEvent** events, size_t count) {
		DCHECK(count) << "Cannot wait on no events";
		internal::ScopedBlockingCallWithBaseSyncPrimitives scoped_blocking_call(
			BlockingType::MAY_BLOCK);
		// Record an event (the first) that this thread is blocking upon.
		debug::ScopedEventWaitActivity event_activity(events[0]);

		HANDLE handles[MAXIMUM_WAIT_OBJECTS];
		CHECK_LE(count, static_cast<size_t>(MAXIMUM_WAIT_OBJECTS))
			<< "Can only wait on " << MAXIMUM_WAIT_OBJECTS << " with WaitMany";

		for (size_t i = 0; i < count; ++i)
			handles[i] = events[i]->handle();

		// The cast is safe because count is small - see the CHECK above.
		const auto result =
			WaitForMultipleObjects(static_cast<DWORD>(count),
				handles,
				FALSE,      // don't wait for all the objects
				INFINITE);  // no timeout
		if (result >= WAIT_OBJECT_0 + count) {
			DPLOG(FATAL) << "WaitForMultipleObjects failed";
			return 0;
		}

		return result - WAIT_OBJECT_0;
	}

}  // namespace base
