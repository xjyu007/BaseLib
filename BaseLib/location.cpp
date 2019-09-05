// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "location.h"
#include <intrin.h>
#include "compiler_specific.h"
#include "strings/string_number_conversions.h"
#include "strings/stringprintf.h"

namespace base {

	Location::Location() = default;
	Location::Location(const Location& other) = default;

	Location::Location(const char* file_name, const void* program_counter) : file_name_(file_name), program_counter_(program_counter) {
	}

	Location::Location(const char* function_name, const char* file_name, int line_number, const void* program_counter)
		: function_name_(function_name), file_name_(file_name), line_number_(line_number), program_counter_(program_counter) {
#if !defined(OS_NACL)
		// The program counter should not be null except in a default constructed
		// (empty) Location object. This value is used for identity, so if it doesn't
		// uniquely identify a location, things will break.
		//
		// The program counter isn't supported in NaCl so location objects won't work
		// properly in that context.
		DCHECK(program_counter);
#endif
	}

	std::string Location::ToString() const {
		if (has_source_info()) {
			return std::string(function_name_) + "@" + file_name_ + ":" +
				NumberToString(line_number_);
		}
		return StringPrintf("pc:%p", program_counter_);
	}

#define RETURN_ADDRESS() _ReturnAddress()

	// static
	NOINLINE Location Location::CreateFromHere(const char* file_name) {
		return Location(file_name, RETURN_ADDRESS());
	}

	// static
	NOINLINE Location Location::CreateFromHere(const char* function_name, const char* file_name, int line_number) {
		return Location(function_name, file_name, line_number, RETURN_ADDRESS());
	}

#/*if SUPPORTS_LOCATION_BUILTINS && BUILDFLAG(ENABLE_LOCATION_SOURCE)
	// static
	NOINLINE Location Location::Current(const char* function_name,
		const char* file_name,
		int line_number) {
		return Location(function_name, file_name, line_number, RETURN_ADDRESS());
	}
#el*/if SUPPORTS_LOCATION_BUILTINS
	// static
	NOINLINE Location Location::Current(const char* file_name) {
		return Location(file_name, RETURN_ADDRESS());
	}
#else
	// static
	NOINLINE Location Location::Current() {
		return Location(nullptr, RETURN_ADDRESS());
	}
#endif

	//------------------------------------------------------------------------------
	NOINLINE const void* GetProgramCounter() {
		return RETURN_ADDRESS();
	}

} // namespace base