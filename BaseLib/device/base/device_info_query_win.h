// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Windows.h>
#include <SetupAPI.h>

#include <string>

#include "macros.h"
#include "base_export.h"

namespace device {

	// Wraps HDEVINFO and SP_DEVINFO_DATA into a class that can automatically
	// release them. Provides interfaces that can add a device using its
	// device path, get device info and get device string property.
	class BASE_EXPORT DeviceInfoQueryWin {
	public:
		DeviceInfoQueryWin();
		~DeviceInfoQueryWin();

		// Add a device to |device_info_list_| using its |device_path| so that
		// its device info can be retrieved.
		bool AddDevice(const std::string& device_path) const;
		// Get the device info and store it into |device_info_data_|, this function
		// should be called at most once.
		bool GetDeviceInfo();
		// Get device string property and store it into |property_buffer|.
		bool GetDeviceStringProperty(const DEVPROPKEY& property,
			std::string* property_buffer);

		bool device_info_list_valid() const	{
			return device_info_list_ != INVALID_HANDLE_VALUE;
		}

	private:
		HDEVINFO device_info_list_ = INVALID_HANDLE_VALUE;
		// When device_info_data_.cbSize != 0, |device_info_data_| is valid.
		SP_DEVINFO_DATA device_info_data_{};

		DISALLOW_COPY_AND_ASSIGN(DeviceInfoQueryWin);
	};

}  // namespace device
