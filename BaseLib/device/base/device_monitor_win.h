// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Windows.h>

#include "observer_list.h"
#include "base_export.h"

namespace device {

	// Use an instance of this class to observe devices being added and removed
	// from the system, matched by device interface GUID.
	class BASE_EXPORT DeviceMonitorWin {
	public:
		class BASE_EXPORT Observer {
		public:
			virtual void OnDeviceAdded(const GUID& class_guid,
				const std::string& device_path);
			virtual void OnDeviceRemoved(const GUID& class_guid,
				const std::string& device_path);
		};

		~DeviceMonitorWin();

		static DeviceMonitorWin* GetForDeviceInterface(const GUID& guid);
		static DeviceMonitorWin* GetForAllInterfaces();

		void AddObserver(Observer* observer);
		void RemoveObserver(Observer* observer);

	private:
		friend class DeviceMonitorMessageWindow;

		DeviceMonitorWin();

		void NotifyDeviceAdded(const GUID& class_guid,
			const std::string& device_path);
		void NotifyDeviceRemoved(const GUID& class_guid,
			const std::string& device_path);

		base::ObserverList<Observer>::Unchecked observer_list_;
	};

}  // namespace device
