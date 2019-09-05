// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>

#include "callback.h"
#include "device/serial/io_buffer.h"
#include "device/serial/serial_info.h"

namespace device {

	// A fixed-size read-only buffer. The data-reader should call Done() when it is
	// finished reading bytes from the buffer. Alternatively, the reader can report
	// an error by calling DoneWithError() with the number of bytes read and the
	// error it wishes to report.
	class ReadOnlyBuffer {
	public:
		virtual ~ReadOnlyBuffer();
		virtual const uint8_t* GetData() = 0;
		virtual uint32_t GetSize() = 0;
		virtual void Done(uint32_t bytes_read) = 0;
		virtual void DoneWithError(uint32_t bytes_read, int32_t error) = 0;
	};

	// A fixed-size writable buffer. The data-writer should call Done() when it is
	// finished writing bytes to the buffer. Alternatively, the writer can report
	// an error by calling DoneWithError() with the number of bytes written and the
	// error it wishes to report.
	class WritableBuffer {
	public:
		virtual ~WritableBuffer();
		virtual char* GetData() = 0;
		virtual uint32_t GetSize() = 0;
		virtual void Done(uint32_t bytes_written) = 0;
		virtual void DoneWithError(uint32_t bytes_written, int32_t error) = 0;
	};

	// A useful basic implementation of a ReadOnlyBuffer in which the data is
	// initialized via a character vector.
	class SendBuffer : public ReadOnlyBuffer {
	public:
		using SendCompleteCallback =
			base::OnceCallback<void(uint32_t, SerialSendError)>;
		SendBuffer(const uint8_t* buffer,
			uint32_t size,
			SendCompleteCallback callback);
		~SendBuffer() override;

		const uint8_t* GetData() override;
		uint32_t GetSize() override;
		void Done(uint32_t bytes_read) override;
		void DoneWithError(uint32_t bytes_read, int32_t error) override;

	private:
		const uint8_t* data_;
		uint32_t size_;
		SendCompleteCallback callback_;
	};

	// A useful basic implementation of a WritableBuffer in which the data is
	// stored in a net::IOBuffer.
	class ReceiveBuffer : public WritableBuffer {
	public:
		using ReceiveCompleteCallback =
			base::OnceCallback<void(uint32_t, SerialReceiveError)>;
		ReceiveBuffer(char* buffer, uint32_t size, ReceiveCompleteCallback callback);
		~ReceiveBuffer() override;

		char* GetData() override;
		uint32_t GetSize() override;
		void Done(uint32_t bytes_written) override;
		void DoneWithError(uint32_t bytes_written, int32_t error) override;

	private:
		char* buffer_;
		const uint32_t size_;
		ReceiveCompleteCallback callback_;
	};

}  // namespace device
