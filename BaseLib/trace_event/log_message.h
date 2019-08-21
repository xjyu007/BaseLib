// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "macros.h"
#include "trace_event/trace_event_impl.h"

namespace base {

	class Value;

	namespace trace_event {

		class BASE_EXPORT LogMessage : public ConvertableToTraceFormat {
		public:
			LogMessage(const char* file, std::string_view message, int line);
			~LogMessage() override;

			// ConvertableToTraceFormat class implementation.
			void AppendAsTraceFormat(std::string* out) const override;
			bool AppendToProto(ProtoAppender* appender) override;

			void EstimateTraceMemoryOverhead(TraceEventMemoryOverhead* overhead) override;

			[[nodiscard]] const char* file() const { return file_; }
			[[nodiscard]] const std::string& message() const { return message_; }
			[[nodiscard]] int line_number() const { return line_number_; }

		private:
			const char* file_;
			std::string message_;
			int line_number_;
			DISALLOW_COPY_AND_ASSIGN(LogMessage);
		};

	}  // namespace trace_event
}  // namespace base
