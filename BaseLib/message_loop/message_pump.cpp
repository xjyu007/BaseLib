// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "message_loop/message_pump.h"

#include "message_loop/message_pump_default.h"
#include "message_loop/message_pump_for_io.h"
#include "message_loop/message_pump_for_ui.h"

namespace base {

	namespace {

		MessagePump::MessagePumpFactory* message_pump_for_ui_factory_ = nullptr;

	}  // namespace

	MessagePump::MessagePump() = default;

	MessagePump::~MessagePump() = default;

	void MessagePump::SetTimerSlack(TimerSlack) {
	}

	// static
	void MessagePump::OverrideMessagePumpForUIFactory(MessagePumpFactory* factory) {
		DCHECK(!message_pump_for_ui_factory_);
		message_pump_for_ui_factory_ = factory;
	}

	// static
	bool MessagePump::IsMessagePumpForUIFactoryOveridden() {
		return message_pump_for_ui_factory_ != nullptr;
	}

	// static
	std::unique_ptr<MessagePump> MessagePump::Create(MessagePumpType type) {
		switch (type) {
		case MessagePumpType::UI:
			if (message_pump_for_ui_factory_)
				return message_pump_for_ui_factory_();
			return std::make_unique<MessagePumpForUI>();

		case MessagePumpType::IO:
			return std::make_unique<MessagePumpForIO>();

		case MessagePumpType::UI_WITH_WM_QUIT_SUPPORT: {
			auto pump = std::make_unique<MessagePumpForUI>();
			pump->EnableWmQuit();
			return pump;
		}

		case MessagePumpType::CUSTOM:
			NOTREACHED();
			return nullptr;

		case MessagePumpType::DEFAULT:
			return std::make_unique<MessagePumpDefault>();
		}
		return nullptr;
	}

}  // namespace base
