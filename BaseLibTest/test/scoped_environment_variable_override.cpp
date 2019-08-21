// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "test/scoped_environment_variable_override.h"
#include "environment.h"

namespace base::test
{

		ScopedEnvironmentVariableOverride::ScopedEnvironmentVariableOverride(
			const std::string& variable_name,
			const std::string& value)
			: environment_(Environment::Create()),
			variable_name_(variable_name),
			overridden_(false),
			was_set_(false) {
			was_set_ = environment_->GetVar(variable_name, &old_value_);
			overridden_ = environment_->SetVar(variable_name, value);
		}

		ScopedEnvironmentVariableOverride::~ScopedEnvironmentVariableOverride() {
			if (overridden_) {
				if (was_set_)
					environment_->SetVar(variable_name_, old_value_);
				else
					environment_->UnSetVar(variable_name_);
			}
		}
} // namespace base
