// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <optional>

#include "device/serial/serial_info.h"
#include "device/serial/serial_io_handler.h"

namespace base {
	class FilePath;
}

namespace device {

	// Discovers and enumerates serial devices available to the host.
	class SerialDeviceEnumerator {
	public:
		using TokenPathMap = std::map<base::UnguessableToken, base::FilePath>;

		static std::unique_ptr<SerialDeviceEnumerator> Create();

		SerialDeviceEnumerator();
		virtual ~SerialDeviceEnumerator();

		virtual std::vector<SerialPortInfo*> GetDevices() = 0;

		virtual std::optional<base::FilePath> GetPathFromToken(
			const base::UnguessableToken& token);

	protected:
		const base::UnguessableToken& GetTokenFromPath(const base::FilePath& path);

	private:
		TokenPathMap token_path_map_;
	};

}  // namespace device
