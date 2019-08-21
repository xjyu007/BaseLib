// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "i18n/base_i18n_export.h"

namespace switches {

	BASE_I18N_EXPORT extern const char kForceUIDirection[];
	BASE_I18N_EXPORT extern const char kForceTextDirection[];

	// kForce*Direction choices for the switches above.
	BASE_I18N_EXPORT extern const char kForceDirectionLTR[];
	BASE_I18N_EXPORT extern const char kForceDirectionRTL[];

}  // namespace switches
