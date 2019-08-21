// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <unordered_set>

#include "base_export.h"
#include "macros.h"
#include "trace_event/trace_event_filter.h"

namespace base {
	namespace trace_event {

		class TraceEvent;

		// Filters trace events by checking the full name against a whitelist.
		// The current implementation is quite simple and dumb and just uses a
		// hashtable which requires char* to std::string conversion. It could be smarter
		// and use a bloom filter trie. However, today this is used too rarely to
		// justify that cost.
		class BASE_EXPORT EventNameFilter : public TraceEventFilter {
		public:
			using EventNamesWhitelist = std::unordered_set<std::string>;
			static const char kName[];

			EventNameFilter(std::unique_ptr<EventNamesWhitelist>);
			~EventNameFilter() override;

			// TraceEventFilter implementation.
			[[nodiscard]] bool FilterTraceEvent(const TraceEvent&) const override;

		private:
			std::unique_ptr<const EventNamesWhitelist> event_names_whitelist_;

			DISALLOW_COPY_AND_ASSIGN(EventNameFilter);
		};

	}  // namespace trace_event
}  // namespace base
