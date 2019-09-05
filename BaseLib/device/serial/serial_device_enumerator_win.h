// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vector>

#include "macros.h"
#include <optional>
#include "device/serial/serial_device_enumerator.h"

namespace device {

	// Discovers and enumerates serial devices available to the host.
	class SerialDeviceEnumeratorWin : public SerialDeviceEnumerator {
	public:
		SerialDeviceEnumeratorWin();
		~SerialDeviceEnumeratorWin() override;

		// Implementation for SerialDeviceEnumerator.
		std::vector<SerialPortInfo*> GetDevices() override;

		// Searches for the COM port in the device's friendly name and returns the
		// appropriate device path or nullopt if the input did not contain a valid
		// name.
		static std::optional<base::FilePath> GetPath(
			const std::string& friendly_name);

	private:
		std::vector<SerialPortInfo*> GetDevicesNew();
		std::vector<SerialPortInfo*> GetDevicesOld();

		DISALLOW_COPY_AND_ASSIGN(SerialDeviceEnumeratorWin);
	};

}  // namespace device
