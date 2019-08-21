#pragma once

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <string>

#include "base_export.h"
#include "build_config.h"

namespace base {
	class BASE_EXPORT Environment {
	public:
		virtual ~Environment();

		// Returns the appropriate platform-specific instance.
		static std::unique_ptr<Environment> Create();

		// Gets an environment variable's value and stores it in |result|.
		// Returns false if the key is unset.
		virtual bool GetVar(std::string_view variable_name, std::string* result) = 0;

		// Syntactic sugar for GetVar(variable_name, nullptr);
		virtual bool HasVar(std::string_view variable_name);

		// Returns true on success, otherwise returns false. This method should not
		// be called in a multi-threaded process.
		virtual bool SetVar(std::string_view variable_name,	const std::string& new_value) = 0;

		// Returns true on success, otherwise returns false. This method should not
		// be called in a multi-threaded process.
		virtual bool UnSetVar(std::string_view variable_name) = 0;
	};

	using NativeEnvironmentString = std::wstring;
	using EnvironmentMap = std::map<NativeEnvironmentString, NativeEnvironmentString>;
}  // namespace base

