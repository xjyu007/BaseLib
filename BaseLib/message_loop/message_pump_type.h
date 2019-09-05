// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

namespace base {

	// A MessagePump has a particular type, which indicates the set of
	// asynchronous events it may process in addition to tasks and timers.

	enum class MessagePumpType {
		// This type of pump only supports tasks and timers.
		DEFAULT,

		// This type of pump also supports native UI events (e.g., Windows
		// messages).
		UI,

		// User provided implementation of MessagePump interface
		CUSTOM,

		// This type of pump also supports asynchronous IO.
		IO,

		// This type of pump supports WM_QUIT messages in addition to other native
		// UI events. This is only for use on windows.
		UI_WITH_WM_QUIT_SUPPORT,
	};

}  // namespace base
