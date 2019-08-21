// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "macros.h"
#include "message_loop/message_pump.h"
#include "synchronization/waitable_event.h"
#include "time/time.h"

namespace base {

	class BASE_EXPORT MessagePumpDefault : public MessagePump {
	public:
		MessagePumpDefault();
		~MessagePumpDefault() override;

		// MessagePump methods:
		void Run(Delegate* delegate) override;
		void Quit() override;
		void ScheduleWork() override;
		void ScheduleDelayedWork(const TimeTicks& delayed_work_time) override;

	private:
		// This flag is set to false when Run should return.
		bool keep_running_;

		// Used to sleep until there is more work to do.
		WaitableEvent event_;

		DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
	};

}  // namespace base
