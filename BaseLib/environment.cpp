// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "environment.h"

#include "memory/ptr_util.h"
#include "strings/string_util.h"
#include "strings/utf_string_conversions.h"
#include "build_config.h"

#include <Windows.h>

namespace base {
	namespace {
		class EnvironmentImpl : public Environment {
		public:
			bool GetVar(std::string_view variable_name, std::string* result) override {
				if (GetVarImpl(variable_name, result))
					return true;

				// Some commonly used variable names are uppercase while others
				// are lowercase, which is inconsistent. Let's try to be helpful
				// and look for a variable name with the reverse case.
				// I.e. HTTP_PROXY may be http_proxy for some users/systems.
				const auto first_char = variable_name[0];
				std::string alternate_case_var;
				if (IsAsciiLower(first_char))
					alternate_case_var = ToUpperASCII(variable_name);
				else if (IsAsciiUpper(first_char))
					alternate_case_var = ToLowerASCII(variable_name);
				else
					return false;
				return GetVarImpl(alternate_case_var, result);
			}

			bool SetVar(std::string_view variable_name,
				const std::string& new_value) override {
				return SetVarImpl(variable_name, new_value);
			}

			bool UnSetVar(std::string_view variable_name) override {
				return UnSetVarImpl(variable_name);
			}

		private:
			bool GetVarImpl(std::string_view variable_name, std::string* result) const {
				const auto value_length = ::GetEnvironmentVariableW(UTF8ToWide(variable_name).c_str(), nullptr, 0);
				if (value_length == 0)
					return false;
				if (result) {
					const std::unique_ptr<wchar_t[]> value(new wchar_t[value_length]);
					::GetEnvironmentVariableW(UTF8ToWide(variable_name).c_str(), value.get(), value_length);
					*result = WideToUTF8(value.get());
				}
				return true;
			}

			[[nodiscard]] bool SetVarImpl(std::string_view variable_name, const std::string& new_value) const {
				// On success, a nonzero value is returned.
				return !!SetEnvironmentVariable(UTF8ToWide(variable_name).c_str(), UTF8ToWide(new_value).c_str());
			}

			[[nodiscard]] bool UnSetVarImpl(std::string_view variable_name) const {
				// On success, a nonzero value is returned.
				return !!SetEnvironmentVariable(UTF8ToWide(variable_name).c_str(), nullptr);
			}
		};

	}  // namespace

	Environment::~Environment() = default;

	// static
	std::unique_ptr<Environment> Environment::Create() {
		return std::make_unique<EnvironmentImpl>();
	}

	bool Environment::HasVar(std::string_view variable_name) {
		return GetVar(variable_name, nullptr);
	}

}  // namespace base
