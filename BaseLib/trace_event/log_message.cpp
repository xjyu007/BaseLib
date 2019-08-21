// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "trace_event/log_message.h"
#include "trace_event/trace_event_memory_overhead.h"

#include <string>

#include "json/string_escape.h"
#include "strings/stringprintf.h"

namespace base::trace_event {

	LogMessage::LogMessage(const char* file, std::string_view message, int line)
		: file_(file), message_(message), line_number_(line) {}

	LogMessage::~LogMessage() = default;

	void LogMessage::AppendAsTraceFormat(std::string* out) const {
		out->append("{");
		out->append(base::StringPrintf(R"("line":"%d",)", line_number_));
		out->append("\"message\":");
		EscapeJSONString(message_, true, out);
		out->append(",");
		out->append(base::StringPrintf(R"("file":"%s")", file_));
		out->append("}");
	}

	void LogMessage::EstimateTraceMemoryOverhead(TraceEventMemoryOverhead* overhead) {
		overhead->Add(TraceEventMemoryOverhead::kOther, sizeof(*this));
		overhead->AddString(message_);
	}

	bool LogMessage::AppendToProto(ProtoAppender* appender) {
		// LogMessage is handled in a special way in
		// track_event_thread_local_event_sink.cc in the function |AddTraceEvent|, so
		// this call should never happen.
		NOTREACHED();
		return false;
	}
} // namespace base
