// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "debug_/stack_trace.h"

#include <string.h>

#include <algorithm>
#include <sstream>

#include "logging.h"
#include "stl_util.h"

#pragma comment(lib,"DbgHelp.lib")

namespace base::debug {
	StackTrace::StackTrace() : StackTrace(base::size(trace_)) {}

	StackTrace::StackTrace(size_t count) {
		count_ = CollectStackTrace(trace_, std::min(count, base::size(trace_)));
	}

	StackTrace::StackTrace(const void* const* trace, size_t count) {
		count = std::min(count, base::size(trace_));
		if (count)
			memcpy(trace_, trace, count * sizeof(trace_[0]));
		count_ = count;
	}

	const void* const* StackTrace::Addresses(size_t* count) const {
		*count = count_;
		if (count_)
			return trace_;
		return nullptr;
	}

	void StackTrace::Print() const {
		PrintWithPrefix(nullptr);
	}

	void StackTrace::OutputToStream(std::ostream* os) const {
		OutputToStreamWithPrefix(os, nullptr);
	}

	std::string StackTrace::ToString() const {
		return ToStringWithPrefix(nullptr);
	}
	std::string StackTrace::ToStringWithPrefix(const char* prefix_string) const {
		std::stringstream stream;
#if !defined(__UCLIBC__) && !defined(_AIX)
		OutputToStreamWithPrefix(&stream, prefix_string);
#endif
		return stream.str();
	}

	std::ostream& operator<<(std::ostream& os, const StackTrace& s) {
#if !defined(__UCLIBC__) & !defined(_AIX)
		s.OutputToStream(&os);
#else
		os << "StackTrace::OutputToStream not implemented.";
#endif
		return os;
	}
} // namespace base
