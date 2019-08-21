#include <windows.h>
#include "sys_string_conversions.h"

namespace base {
	// Do not assert in this function since it is used by the asssertion code!
	std::string SysWideToUTF8(const std::wstring& wide) {
		return SysWideToMultiByte(wide, CP_UTF8);
	}

	// Do not assert in this function since it is used by the asssertion code!
	std::wstring SysUTF8ToWide(std::string_view utf8) {
		return SysMultiByteToWide(utf8, CP_UTF8);
	}

	std::string SysWideToNativeMB(const std::wstring& wide) {
		return SysWideToMultiByte(wide, CP_ACP);
	}

	std::wstring SysNativeMBToWide(std::string_view native_mb) {
		return SysMultiByteToWide(native_mb, CP_ACP);
	}

	// Do not assert in this function since it is used by the asssertion code!
	std::wstring SysMultiByteToWide(std::string_view mb, uint32_t code_page) {
		if (mb.empty())
			return std::wstring();

		const auto mb_length = static_cast<int>(mb.length());
		// Compute the length of the buffer.
		const auto charcount = MultiByteToWideChar(code_page, 0,
			mb.data(), mb_length, NULL, 0);
		if (charcount == 0)
			return std::wstring();

		std::wstring wide;
		wide.resize(charcount);
		MultiByteToWideChar(code_page, 0, mb.data(), mb_length, &wide[0], charcount);

		return wide;
	}

	// Do not assert in this function since it is used by the asssertion code!
	std::string SysWideToMultiByte(const std::wstring& wide, uint32_t code_page) {
		const auto wide_length = static_cast<int>(wide.length());
		if (wide_length == 0)
			return std::string();

		// Compute the length of the buffer we'll need.
		const auto charcount = WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
			NULL, 0, NULL, NULL);
		if (charcount == 0)
			return std::string();

		std::string mb;
		mb.resize(charcount);
		WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
			&mb[0], charcount, NULL, NULL);

		return mb;
	}
}