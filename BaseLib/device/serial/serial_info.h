#pragma once

#include <cstdint>
#include "files/file_path.h"
#include "unguessable_token.h"

namespace device {

	struct SerialPortInfo {
		base::UnguessableToken token;
		base::FilePath path;
		uint16_t vendor_id;
		bool has_vendor_id = false;
		uint16_t product_id;
		bool has_product_id = false;
		std::string display_name;
	};
	
	enum class SerialSendError {
		NONE,
		DISCONNECTED,
		SYSTEM_ERROR,
	};

	enum class SerialReceiveError {
		NONE,
		DISCONNECTED,
		DEVICE_LOST,
		BREAK,
		FRAME_ERROR,
		OVERRUN,
		BUFFER_OVERFLOW,
		PARITY_ERROR,
		SYSTEM_ERROR,
	};

	enum class SerialDataBits {
		NONE,
		SEVEN,
		EIGHT,
	};

	enum class SerialParityBit {
		NONE,
		NO_PARITY,
		ODD,
		EVEN,
	};

	enum class SerialStopBits {
		NONE,
		ONE,
		TWO,
	};

	struct SerialConnectionOptions {
		uint32_t bitrate = 0;
		SerialDataBits data_bits = SerialDataBits::NONE;
		SerialParityBit parity_bit = SerialParityBit::NONE;
		SerialStopBits stop_bits = SerialStopBits::NONE;
		bool cts_flow_control;
		bool has_cts_flow_control = false;
	};

	struct SerialConnectionInfo {
		uint32_t bitrate = 0;
		SerialDataBits data_bits = SerialDataBits::NONE;
		SerialParityBit parity_bit = SerialParityBit::NONE;
		SerialStopBits stop_bits = SerialStopBits::NONE;
		bool cts_flow_control;
	};

	struct SerialHostControlSignals {
		bool dtr;
		bool has_dtr = false;
		bool rts;
		bool has_rts = false;
	};

	struct SerialPortControlSignals {
		bool dcd;
		bool cts;
		bool ri;
		bool dsr;
	};

}
