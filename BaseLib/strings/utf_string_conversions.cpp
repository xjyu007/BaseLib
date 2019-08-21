// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "strings/utf_string_conversions.h"

#include <limits.h>
#include <stdint.h>

#include <type_traits>

#include "bit_cast.h"
#include "strings/string_util.h"
#include "strings/utf_string_conversion_utils.h"
#include "third_party/icu/icu_utf.h"
#include "build_config.h"

namespace base {

	namespace {

		constexpr int32_t kErrorCodePoint = 0xFFFD;

		// Size coefficient ----------------------------------------------------------
		// The maximum number of codeunits in the destination encoding corresponding to
		// one codeunit in the source encoding.

		template <typename SrcChar, typename DestChar>
		struct SizeCoefficient {
			static_assert(sizeof(SrcChar) < sizeof(DestChar),
				"Default case: from a smaller encoding to the bigger one");

			// ASCII symbols are encoded by one codeunit in all encodings.
			static constexpr int value = 1;
		};

		template <>
		struct SizeCoefficient<wchar_t, char> {
			// One UTF-16 codeunit corresponds to at most 3 codeunits in UTF-8.
			static constexpr int value = 3;
		};

		template <typename SrcChar, typename DestChar>
		constexpr int size_coefficient_v =
			SizeCoefficient<std::decay_t<SrcChar>, std::decay_t<DestChar>>::value;

		// UnicodeAppendUnsafe --------------------------------------------------------
		// Function overloads that write code_point to the output string. Output string
		// has to have enough space for the codepoint.

		// Convenience typedef that checks whether the passed in type is integral (i.e.
		// bool, char, int or their extended versions) and is of the correct size.
		template <typename Char, size_t N>
		using EnableIfBitsAre = std::enable_if_t<std::is_integral<Char>::value&&
			CHAR_BIT * sizeof(Char) == N,
			bool>;

		template <typename Char, EnableIfBitsAre<Char, 8> = true>
		void UnicodeAppendUnsafe(Char * out, int32_t * size, uint32_t code_point) {
			CBU8_APPEND_UNSAFE(out, *size, code_point);
		}

		template <typename Char, EnableIfBitsAre<Char, 16> = true>
		void UnicodeAppendUnsafe(Char * out, int32_t * size, uint32_t code_point) {
			CBU16_APPEND_UNSAFE(out, *size, code_point);
		}

		template <typename Char, EnableIfBitsAre<Char, 32> = true>
		void UnicodeAppendUnsafe(Char * out, int32_t * size, uint32_t code_point) {
			out[(*size)++] = code_point;
		}

		// DoUTFConversion ------------------------------------------------------------
		// Main driver of UTFConversion specialized for different Src encodings.
		// dest has to have enough room for the converted text.

		template <typename DestChar>
		bool DoUTFConversion(const char* src,
			int32_t src_len,
			DestChar* dest,
			int32_t* dest_len) {
			bool success = true;

			for (int32_t i = 0; i < src_len;) {
				int32_t code_point;
				CBU8_NEXT(src, i, src_len, code_point);

				if (!IsValidCodepoint(code_point)) {
					success = false;
					code_point = kErrorCodePoint;
				}

				UnicodeAppendUnsafe(dest, dest_len, code_point);
			}

			return success;
		}

		template <typename DestChar>
		bool DoUTFConversion(const wchar_t* src,
			int32_t src_len,
			DestChar* dest,
			int32_t* dest_len) {
			bool success = true;

			auto ConvertSingleChar = [&success](wchar_t in) -> int32_t {
				if (!CBU16_IS_SINGLE(in) || !IsValidCodepoint(in)) {
					success = false;
					return kErrorCodePoint;
				}
				return in;
			};

			int32_t i = 0;

			// Always have another symbol in order to avoid checking boundaries in the
			// middle of the surrogate pair.
			while (i < src_len - 1) {
				int32_t code_point;

				if (CBU16_IS_LEAD(src[i]) && CBU16_IS_TRAIL(src[i + 1])) {
					code_point = CBU16_GET_SUPPLEMENTARY(src[i], src[i + 1]);
					if (!IsValidCodepoint(code_point)) {
						code_point = kErrorCodePoint;
						success = false;
					}
					i += 2;
				}
				else {
					code_point = ConvertSingleChar(src[i]);
					++i;
				}

				UnicodeAppendUnsafe(dest, dest_len, code_point);
			}

			if (i < src_len)
				UnicodeAppendUnsafe(dest, dest_len, ConvertSingleChar(src[i]));

			return success;
		}

		// UTFConversion --------------------------------------------------------------
		// Function template for generating all UTF conversions.
		bool UTFConversion(std::wstring_view src_str, std::string* dest_str) {
			/*if (IsStringASCII(src_str)) {
				dest_str->assign(src_str.begin(), src_str.end());
				return true;
			}*/

			dest_str->resize(src_str.length() * size_coefficient_v<wchar_t, char>);

			// Empty string is ASCII => it OK to call operator[].
			auto* dest = &(*dest_str)[0];

			// ICU requires 32 bit numbers.
			int32_t src_len32 = static_cast<int32_t>(src_str.length());
			int32_t dest_len32 = 0;

			bool res = DoUTFConversion(src_str.data(), src_len32, dest, &dest_len32);

			dest_str->resize(dest_len32);
			dest_str->shrink_to_fit();

			return res;
		}

		bool UTFConversion(std::string_view src_str, std::wstring* dest_str) {
			if (IsStringASCII(src_str)) {
				dest_str->assign(src_str.begin(), src_str.end());
				return true;
			}

			dest_str->resize(src_str.length() * size_coefficient_v<char, wchar_t>);

			// Empty string is ASCII => it OK to call operator[].
			auto* dest = &(*dest_str)[0];

			// ICU requires 32 bit numbers.
			int32_t src_len32 = static_cast<int32_t>(src_str.length());
			int32_t dest_len32 = 0;

			bool res = DoUTFConversion(src_str.data(), src_len32, dest, &dest_len32);

			dest_str->resize(dest_len32);
			dest_str->shrink_to_fit();

			return res;
		}

	}  // namespace

	// UTF-8 <-> Wide --------------------------------------------------------------

	// UTF8ToWide is the same code, regardless of whether wide is 16 or 32 bits

	bool UTF16ToUTF8(const wchar_t* src, size_t src_len, std::string* output) {
		return UTFConversion(std::wstring_view(src, src_len), output);
	}

	std::string UTF16ToUTF8(std::wstring_view utf16) {
		std::string ret;
		// Ignore the success flag of this call, it will do the best it can for
		// invalid input, which is what we want here.
		UTF16ToUTF8(utf16.data(), utf16.length(), &ret);
		return ret;
	}

	bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output) {
		return UTFConversion(std::string_view(src, src_len), output);
	}

	std::wstring UTF8ToWide(std::string_view utf8) {
		std::wstring ret;
		// Ignore the success flag of this call, it will do the best it can for
		// invalid input, which is what we want here.
		UTF8ToWide(utf8.data(), utf8.length(), &ret);
		return ret;
	}

	// Easy case since we can use the "utf" versions we already wrote above.

	bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output) {
		return UTF16ToUTF8(as_u16cstr(src), src_len, output);
	}

	std::string WideToUTF8(std::wstring_view wide) {
		return UTF16ToUTF8(std::wstring_view(as_u16cstr(wide), wide.size()));
	}

	std::wstring ASCIIToUTF16(std::string_view ascii) {
		DCHECK(IsStringASCII(ascii)) << ascii;
		return std::wstring(ascii.begin(), ascii.end());
	}

	std::string UTF16ToASCII(std::wstring_view utf16) {
		std::string ret(UTF16ToUTF8(utf16));
		//DCHECK(IsStringASCII(utf16)) << UTF16ToUTF8(utf16);
		DCHECK(IsStringASCII(ret));
		//return std::string(utf16.begin(), utf16.end());
		return ret;
	}

}  // namespace base
