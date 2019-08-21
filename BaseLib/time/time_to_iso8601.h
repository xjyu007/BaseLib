// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "base_export.h"

namespace base {

	class Time;

	BASE_EXPORT std::string TimeToISO8601(const base::Time& t);

}  // namespace base

