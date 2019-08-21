// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

namespace base
{

	// Amount of timer slack to use for delayed timers.  Increasing timer slack
	// allows the OS to coalesce timers more effectively.
	enum TimerSlack
	{
		// Lowest value for timer slack allowed by OS.
		TIMER_SLACK_NONE,

		// Maximal value for timer slack allowed by OS.
		TIMER_SLACK_MAXIMUM
	};

}  // namespace base
