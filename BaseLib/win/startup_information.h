#pragma once

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <windows.h>

#include "base_export.h"
#include "macros.h"

namespace base {
	namespace win {

		// Manages the lifetime of additional attributes in STARTUPINFOEX.
		class BASE_EXPORT StartupInformation {
		public:
			StartupInformation();

			~StartupInformation();

			// Initialize the attribute list for the specified number of entries.
			bool InitializeProcThreadAttributeList(DWORD attribute_count);

			// Sets one entry in the initialized attribute list.
			// |value| needs to live at least as long as the StartupInformation object
			// this is called on.
			bool UpdateProcThreadAttribute(DWORD_PTR attribute,
				void* value,
				size_t size) const;

			LPSTARTUPINFOW startup_info() { return &startup_info_.StartupInfo; }
			LPSTARTUPINFOW startup_info() const {
				return const_cast<const LPSTARTUPINFOW>(&startup_info_.StartupInfo);
			}

			bool has_extended_startup_info() const {
				return !!startup_info_.lpAttributeList;
			}

		private:
			std::unique_ptr<char[]> attribute_list_;
			STARTUPINFOEXW startup_info_;
			DISALLOW_COPY_AND_ASSIGN(StartupInformation);
		};

	}  // namespace win
}  // namespace base
