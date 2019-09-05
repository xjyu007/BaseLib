// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "macros.h"
#include "message_loop/message_pump_for_io.h"
#include "single_thread_task_runner.h"
#include "threading/thread_task_runner_handle.h"
#include "device/serial/serial_io_handler.h"

namespace device {

	class SerialIoHandlerWin : public SerialIoHandler,
		public base::MessagePumpForIO::IOHandler {
	protected:
		// SerialIoHandler implementation.
		void ReadImpl() override;
		void WriteImpl() override;
		void CancelReadImpl() override;
		void CancelWriteImpl() override;
		bool ConfigurePortImpl() override;
		bool Flush() const override;
		SerialPortControlSignals* GetControlSignals() const override;
		bool SetControlSignals(
			const SerialHostControlSignals& signals) override;
		SerialConnectionInfo* GetPortInfo() const override;
		bool SetBreak() override;
		bool ClearBreak() override;
		bool PostOpen() override;

	private:
		class UiThreadHelper;
		friend class SerialIoHandler;

		explicit SerialIoHandlerWin(
			const base::FilePath& port,
			scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner);
		~SerialIoHandlerWin() override;

		// base::MessagePumpForIO::IOHandler implementation.
		void OnIOCompleted(base::MessagePumpForIO::IOContext* context,
			DWORD bytes_transfered,
			DWORD error) override;

		void OnDeviceRemoved(const std::string& device_path);

		// Context used for asynchronous WaitCommEvent calls.
		std::unique_ptr<base::MessagePumpForIO::IOContext> comm_context_;

		// Context used for overlapped reads.
		std::unique_ptr<base::MessagePumpForIO::IOContext> read_context_;

		// Context used for overlapped writes.
		std::unique_ptr<base::MessagePumpForIO::IOContext> write_context_;

		// Asynchronous event mask state
		DWORD event_mask_;

		// Indicates if a pending read is waiting on initial data arrival via
		// WaitCommEvent, as opposed to waiting on actual ReadFile completion
		// after a corresponding WaitCommEvent has completed.
		bool is_comm_pending_;

		// The helper lives on the UI thread and holds a weak reference back to the
		// handler that owns it.
		UiThreadHelper* helper_;
		base::WeakPtrFactory<SerialIoHandlerWin> weak_factory_{ this };

		DISALLOW_COPY_AND_ASSIGN(SerialIoHandlerWin);
	};

}  // namespace device
