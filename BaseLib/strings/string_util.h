#pragma once

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.

#include <cctype>
#include <cstdarg>   // va_list
#include <cstddef>
#include <cstdint>

#include <initializer_list>
#include <string>
#include <vector>

#include "base_export.h"
#include "bit_cast.h"
#include "compiler_specific.h"
#include "stl_util.h"
#include "build_config.h"

namespace base {

	// C standard-library functions that aren't cross-platform are provided as
	// "base::...", and their prototypes are listed below. These functions are
	// then implemented as inline calls to the platform-specific equivalents in the
	// platform-specific headers.

	// Wrapper for vsnprintf that always null-terminates and always returns the
	// number of characters that would be in an untruncated formatted
	// string, even when truncation occurs.
	int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments) PRINTF_FORMAT(3, 0);

	// Some of these implementations need to be inlined.

	// We separate the declaration from the implementation of this inline
	// function just so the PRINTF_FORMAT works.
	inline int snprintf(char* buffer, size_t size, const char* format, ...) PRINTF_FORMAT(3, 4);
	inline int snprintf(char* buffer, size_t size, const char* format, ...) {
		va_list arguments;
		va_start(arguments, format);
		int result = vsnprintf(buffer, size, format, arguments);
		va_end(arguments);
		return result;
	}

	// BSD-style safe and consistent string copy functions.
	// Copies |src| to |dst|, where |dst_size| is the total allocated size of |dst|.
	// Copies at most |dst_size|-1 characters, and always NULL terminates |dst|, as
	// long as |dst_size| is not 0.  Returns the length of |src| in characters.
	// If the return value is >= dst_size, then the output was truncated.
	// NOTE: All sizes are in number of characters, NOT in bytes.
	BASE_EXPORT size_t strlcpy(char* dst, const char* src, size_t dst_size);
	BASE_EXPORT size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size);

	// Scan a wprintf format string to determine whether it's portable across a
	// variety of systems.  This function only checks that the conversion
	// specifiers used by the format string are supported and have the same meaning
	// on a variety of systems.  It doesn't check for other errors that might occur
	// within a format string.
	//
	// Nonportable conversion specifiers for wprintf are:
	//  - 's' and 'c' without an 'l' length modifier.  %s and %c operate on char
	//     data on all systems except Windows, which treat them as wchar_t data.
	//     Use %ls and %lc for wchar_t data instead.
	//  - 'S' and 'C', which operate on wchar_t data on all systems except Windows,
	//     which treat them as char data.  Use %ls and %lc for wchar_t data
	//     instead.
	//  - 'F', which is not identified by Windows wprintf documentation.
	//  - 'D', 'O', and 'U', which are deprecated and not available on all systems.
	//     Use %ld, %lo, and %lu instead.
	//
	// Note that there is no portable conversion specifier for char data when
	// working with wprintf.
	//
	// This function is intended to be called from base::vswprintf.
	BASE_EXPORT bool IsWprintfFormatPortable(const wchar_t* format);

	// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
	// so we don't want to use it here.
	inline char ToLowerASCII(const char c) {
		return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
	}

	inline wchar_t ToLowerASCII(const wchar_t c) {
		return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
	}

	// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
	// so we don't want to use it here.
	inline char ToUpperASCII(char c) {
		return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
	}
	inline wchar_t ToUpperASCII(wchar_t c) {
		return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
	}

	// Converts the given string to it's ASCII-lowercase equivalent.
	BASE_EXPORT std::string ToLowerASCII(std::string_view str);
	BASE_EXPORT std::wstring ToLowerASCII(std::wstring_view str);

	// Converts the given string to it's ASCII-uppercase equivalent.
	BASE_EXPORT std::string ToUpperASCII(std::string_view str);
	BASE_EXPORT std::wstring ToUpperASCII(std::wstring_view str);

	// Functor for case-insensitive ASCII comparisons for STL algorithms like
	// std::search.
	//
	// Note that a full Unicode version of this functor is not possible to write
	// because case mappings might change the number of characters, depend on
	// context (combining accents), and require handling UTF-16. If you need
	// proper Unicode support, use base::i18n::ToLower/FoldCase and then just
	// use a normal operator== on the result.
	template<typename Char> struct CaseInsensitiveCompareASCII {
	public:
		bool operator()(Char x, Char y) const {
			return ToLowerASCII(x) == ToLowerASCII(y);
		}
	};

	// Like strcasecmp for case-insensitive ASCII characters only. Returns:
	//   -1  (a < b)
	//    0  (a == b)
	//    1  (a > b)
	// (unlike strcasecmp which can return values greater or less than 1/-1). For
	// full Unicode support, use base::i18n::ToLower or base::i18h::FoldCase
	// and then just call the normal string operators on the result.
	BASE_EXPORT int CompareCaseInsensitiveASCII(std::string_view a, std::string_view b);
	BASE_EXPORT int CompareCaseInsensitiveASCII(std::wstring_view a, std::wstring_view b);

	// Equality for ASCII case-insensitive comparisons. For full Unicode support,
	// use base::i18n::ToLower or base::i18h::FoldCase and then compare with either
	// == or !=.
	BASE_EXPORT bool EqualsCaseInsensitiveASCII(std::string_view a, std::string_view b);
	BASE_EXPORT bool EqualsCaseInsensitiveASCII(std::wstring_view a, std::wstring_view b);

#define WHITESPACE_UNICODE \
  0x0009, /* CHARACTER TABULATION */      \
  0x000A, /* LINE FEED (LF) */            \
  0x000B, /* LINE TABULATION */           \
  0x000C, /* FORM FEED (FF) */            \
  0x000D, /* CARRIAGE RETURN (CR) */      \
  0x0020, /* SPACE */                     \
  0x0085, /* NEXT LINE (NEL) */           \
  0x00A0, /* NO-BREAK SPACE */            \
  0x1680, /* OGHAM SPACE MARK */          \
  0x2000, /* EN QUAD */                   \
  0x2001, /* EM QUAD */                   \
  0x2002, /* EN SPACE */                  \
  0x2003, /* EM SPACE */                  \
  0x2004, /* THREE-PER-EM SPACE */        \
  0x2005, /* FOUR-PER-EM SPACE */         \
  0x2006, /* SIX-PER-EM SPACE */          \
  0x2007, /* FIGURE SPACE */              \
  0x2008, /* PUNCTUATION SPACE */         \
  0x2009, /* THIN SPACE */                \
  0x200A, /* HAIR SPACE */                \
  0x2028, /* LINE SEPARATOR */            \
  0x2029, /* PARAGRAPH SEPARATOR */       \
  0x202F, /* NARROW NO-BREAK SPACE */     \
  0x205F, /* MEDIUM MATHEMATICAL SPACE */ \
  0x3000, /* IDEOGRAPHIC SPACE */         \
  0

	// Contains the set of characters representing whitespace in the corresponding
	// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
	// by HTML5, and don't include control characters.
	BASE_EXPORT const wchar_t kWhitespaceWide[] = {
		WHITESPACE_UNICODE
	};  // Includes Unicode.

	BASE_EXPORT const wchar_t kWhitespaceUTF16[] = {
		WHITESPACE_UNICODE
	};  // Includes Unicode.

	BASE_EXPORT const char kWhitespaceASCII[] = {
		0x09,    // CHARACTER TABULATION
		0x0A,    // LINE FEED (LF)
		0x0B,    // LINE TABULATION
		0x0C,    // FORM FEED (FF)
		0x0D,    // CARRIAGE RETURN (CR)
		0x20,    // SPACE
		0
	};

	BASE_EXPORT const wchar_t kWhitespaceASCIIAs16[] = {
		0x09,    // CHARACTER TABULATION
		0x0A,    // LINE FEED (LF)
		0x0B,    // LINE TABULATION
		0x0C,    // FORM FEED (FF)
		0x0D,    // CARRIAGE RETURN (CR)
		0x20,    // SPACE
		0
	};  // No unicode.

	// Null-terminated string representing the UTF-8 byte order mark.
	BASE_EXPORT const char kUtf8ByteOrderMark[] = "\xEF\xBB\xBF";

	// Removes characters in |remove_chars| from anywhere in |input|.  Returns true
	// if any characters were removed.  |remove_chars| must be null-terminated.
	// NOTE: Safe to use the same variable for both |input| and |output|.
	BASE_EXPORT bool RemoveChars(const std::wstring& input, std::wstring_view remove_chars, std::wstring* output);
	BASE_EXPORT bool RemoveChars(const std::string& input, std::string_view remove_chars, std::string* output);

	// Replaces characters in |replace_chars| from anywhere in |input| with
	// |replace_with|.  Each character in |replace_chars| will be replaced with
	// the |replace_with| string.  Returns true if any characters were replaced.
	// |replace_chars| must be null-terminated.
	// NOTE: Safe to use the same variable for both |input| and |output|.
	BASE_EXPORT bool ReplaceChars(const std::wstring& input, std::wstring_view replace_chars, const std::wstring& replace_with, std::wstring* output);
	BASE_EXPORT bool ReplaceChars(const std::string& input, std::string_view replace_chars, const std::string& replace_with, std::string* output);

	enum TrimPositions {
		TRIM_NONE = 0,
		TRIM_LEADING = 1 << 0,
		TRIM_TRAILING = 1 << 1,
		TRIM_ALL = TRIM_LEADING | TRIM_TRAILING,
	};

	// Removes characters in |trim_chars| from the beginning and end of |input|.
	// The 8-bit version only works on 8-bit characters, not UTF-8. Returns true if
	// any characters were removed.
	//
	// It is safe to use the same variable for both |input| and |output| (this is
	// the normal usage to trim in-place).
	BASE_EXPORT bool TrimString(const std::wstring& input, std::wstring_view trim_chars, std::wstring* output);
	BASE_EXPORT bool TrimString(const std::string& input, std::string_view trim_chars, std::string* output);

	// std::string_view versions of the above. The returned pieces refer to the original
	// buffer.
	BASE_EXPORT std::wstring_view TrimString(std::wstring_view input, std::wstring_view trim_chars, TrimPositions positions);
	BASE_EXPORT std::string_view TrimString(std::string_view input, std::string_view trim_chars, TrimPositions positions);

	// Truncates a string to the nearest UTF-8 character that will leave
	// the string less than or equal to the specified byte size.
	BASE_EXPORT void TruncateUTF8ToByteSize(const std::string& input, size_t byte_size, std::string* output);

#if defined(WCHAR_T_IS_UTF16)
	// Utility functions to access the underlying string buffer as a wide char
	// pointer.
	inline wchar_t* as_writable_wcstr(wchar_t* str) {
		return bit_cast<wchar_t*>(str);
	}

	inline wchar_t* as_writable_wcstr(std::wstring& str) {
		return bit_cast<wchar_t*>(data(str));
	}

	inline const wchar_t* as_wcstr(const wchar_t* str) {
		return bit_cast<const wchar_t*>(str);
	}

	inline const wchar_t* as_wcstr(std::wstring_view str) {
		return bit_cast<const wchar_t*>(str.data());
	}

	// Utility functions to access the underlying string buffer as a wchar_t pointer.
	inline wchar_t* as_writable_u16cstr(wchar_t* str) {
		return bit_cast<wchar_t*>(str);
	}

	inline wchar_t* as_writable_u16cstr(std::wstring& str) {
		return bit_cast<wchar_t*>(data(str));
	}

	inline const wchar_t* as_u16cstr(const wchar_t* str) {
		return bit_cast<const wchar_t*>(str);
	}

	inline const wchar_t* as_u16cstr(std::wstring_view str) {
		return bit_cast<const wchar_t*>(str.data());
	}
#endif  // defined(WCHAR_T_IS_UTF16)

	// Trims any whitespace from either end of the input string.
	//
	// The std::string_view versions return a substring referencing the input buffer.
	// The ASCII versions look only for ASCII whitespace.
	//
	// The std::string versions return where whitespace was found.
	// NOTE: Safe to use the same variable for both input and output.
	BASE_EXPORT TrimPositions TrimWhitespace(const std::wstring& input, TrimPositions positions, std::wstring* output);
	BASE_EXPORT std::wstring_view TrimWhitespace(std::wstring_view input, TrimPositions positions);
	BASE_EXPORT TrimPositions TrimWhitespaceASCII(const std::string& input, TrimPositions positions, std::string* output);
	BASE_EXPORT std::string_view TrimWhitespaceASCII(std::string_view input, TrimPositions positions);

	// Searches for CR or LF characters.  Removes all contiguous whitespace
	// strings that contain them.  This is useful when trying to deal with text
	// copied from terminals.
	// Returns |text|, with the following three transformations:
	// (1) Leading and trailing whitespace is trimmed.
	// (2) If |trim_sequences_with_line_breaks| is true, any other whitespace
	//     sequences containing a CR or LF are trimmed.
	// (3) All other whitespace sequences are converted to single spaces.
	BASE_EXPORT std::wstring CollapseWhitespace(const std::wstring& text, bool trim_sequences_with_line_breaks);
	BASE_EXPORT std::string CollapseWhitespaceASCII(const std::string& text, bool trim_sequences_with_line_breaks);

	// Returns true if |input| is empty or contains only characters found in
	// |characters|.
	BASE_EXPORT bool ContainsOnlyChars(std::string_view input, std::string_view characters);
	BASE_EXPORT bool ContainsOnlyChars(std::wstring_view input, std::wstring_view characters);

	// Returns true if the specified string matches the criteria. How can a wide
	// string be 8-bit or UTF8? It contains only characters that are < 256 (in the
	// first case) or characters that use only 8-bits and whose 8-bit
	// representation looks like a UTF-8 string (the second case).
	//
	// Note that IsStringUTF8 checks not only if the input is structurally
	// valid but also if it doesn't contain any non-character codepoint
	// (e.g. U+FFFE). It's done on purpose because all the existing callers want
	// to have the maximum 'discriminating' power from other encodings. If
	// there's a use case for just checking the structural validity, we have to
	// add a new function for that.
	//
	// IsStringASCII assumes the input is likely all ASCII, and does not leave early
	// if it is not the case.
	BASE_EXPORT bool IsStringUTF8(std::string_view str);
	BASE_EXPORT bool IsStringASCII(std::string_view str);
	BASE_EXPORT bool IsStringASCII(std::wstring_view str);
#if defined(WCHAR_T_IS_UTF32)
	BASE_EXPORT bool IsStringASCII(Wstd::string_view str);
#endif

	// Compare the lower-case form of the given string against the given
	// previously-lower-cased ASCII string (typically a constant).
	BASE_EXPORT bool LowerCaseEqualsASCII(std::string_view str, std::string_view lowecase_ascii);
	BASE_EXPORT bool LowerCaseEqualsASCII(std::wstring_view str, std::string_view lowecase_ascii);

	// Performs a case-sensitive string compare of the given 16-bit string against
	// the given 8-bit ASCII string (typically a constant). The behavior is
	// undefined if the |ascii| string is not ASCII.
	BASE_EXPORT bool EqualsASCII(std::wstring_view str, std::string_view ascii);

	// Indicates case sensitivity of comparisons. Only ASCII case insensitivity
	// is supported. Full Unicode case-insensitive conversions would need to go in
	// base/i18n so it can use ICU.
	//
	// If you need to do Unicode-aware case-insensitive StartsWith/EndsWith, it's
	// best to call base::i18n::ToLower() or base::i18n::FoldCase() (see
	// base/i18n/case_conversion.h for usage advice) on the arguments, and then use
	// the results to a case-sensitive comparison.
	enum class CompareCase {
		SENSITIVE,
		INSENSITIVE_ASCII,
	};

	BASE_EXPORT bool StartsWith(std::string_view str, std::string_view search_for, CompareCase case_sensitivity);
	BASE_EXPORT bool StartsWith(std::wstring_view str, std::wstring_view search_for, CompareCase case_sensitivity);
	BASE_EXPORT bool EndsWith(std::string_view str, std::string_view search_for, CompareCase case_sensitivity);
	BASE_EXPORT bool EndsWith(std::wstring_view str, std::wstring_view search_for, CompareCase case_sensitivity);

	// Determines the type of ASCII character, independent of locale (the C
	// library versions will change based on locale).
	template <typename Char>
	inline bool IsAsciiWhitespace(Char c) {
		return c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f';
	}
	template <typename Char>
	inline bool IsAsciiAlpha(Char c) {
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}
	template <typename Char>
	inline bool IsAsciiUpper(Char c) {
		return c >= 'A' && c <= 'Z';
	}
	template <typename Char>
	inline bool IsAsciiLower(Char c) {
		return c >= 'a' && c <= 'z';
	}
	template <typename Char>
	inline bool IsAsciiDigit(Char c) {
		return c >= '0' && c <= '9';
	}

	template <typename Char>
	inline bool IsHexDigit(Char c) {
		return (c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'F') ||
			(c >= 'a' && c <= 'f');
	}

	// Returns the integer corresponding to the given hex character. For example:
	//    '4' -> 4
	//    'a' -> 10
	//    'B' -> 11
	// Assumes the input is a valid hex character. DCHECKs in debug builds if not.
	BASE_EXPORT char HexDigitToInt(wchar_t c);

	// Returns true if it's a Unicode whitespace character.
	BASE_EXPORT bool IsUnicodeWhitespace(wchar_t c);

	// Return a byte string in human-readable format with a unit suffix. Not
	// appropriate for use in any UI; use of FormatBytes and friends in ui/base is
	// highly recommended instead. TODO(avi): Figure out how to get callers to use
	// FormatBytes instead; remove this.
	BASE_EXPORT std::wstring FormatBytesUnlocalized(int64_t bytes);

	// Starting at |start_offset| (usually 0), replace the first instance of
	// |find_this| with |replace_with|.
	BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
		std::wstring* str,
		size_t start_offset,
		std::wstring_view find_this,
		std::wstring_view replace_with);
	BASE_EXPORT void ReplaceFirstSubstringAfterOffset(
		std::string* str,
		size_t start_offset,
		std::string_view find_this,
		std::string_view replace_with);

	// Starting at |start_offset| (usually 0), look through |str| and replace all
	// instances of |find_this| with |replace_with|.
	//
	// This does entire substrings; use std::replace in <algorithm> for single
	// characters, for example:
	//   std::replace(str.begin(), str.end(), 'a', 'b');
	BASE_EXPORT void ReplaceSubstringsAfterOffset(std::wstring* str, size_t start_offset, std::wstring_view find_this, std::wstring_view replace_with);
	BASE_EXPORT void ReplaceSubstringsAfterOffset(std::string* str, size_t start_offset, std::string_view find_this, std::string_view replace_with);

	// Reserves enough memory in |str| to accommodate |length_with_null| characters,
	// sets the size of |str| to |length_with_null - 1| characters, and returns a
	// pointer to the underlying contiguous array of characters.  This is typically
	// used when calling a function that writes results into a character array, but
	// the caller wants the data to be managed by a string-like object.  It is
	// convenient in that is can be used inline in the call, and fast in that it
	// avoids copying the results of the call from a char* into a string.
	//
	// |length_with_null| must be at least 2, since otherwise the underlying string
	// would have size 0, and trying to access &((*str)[0]) in that case can result
	// in a number of problems.
	//
	// Internally, this takes linear time because the resize() call 0-fills the
	// underlying array for potentially all
	// (|length_with_null - 1| * sizeof(string_type::value_type)) bytes.  Ideally we
	// could avoid this aspect of the resize() call, as we expect the caller to
	// immediately write over this memory, but there is no other way to set the size
	// of the string, and not doing that will mean people who access |str| rather
	// than str.c_str() will get back a string of whatever size |str| had on entry
	// to this function (probably 0).
	BASE_EXPORT char* WriteInto(std::string* str, size_t length_with_null);
	BASE_EXPORT wchar_t* WriteInto(std::wstring* str, size_t length_with_null);

	// Does the opposite of SplitString()/SplitStringPiece(). Joins a vector or list
	// of strings into a single string, inserting |separator| (which may be empty)
	// in between all elements.
	//
	// If possible, callers should build a vector of StringPieces and use the
	// std::string_view variant, so that they do not create unnecessary copies of
	// strings. For example, instead of using SplitString, modifying the vector,
	// then using JoinString, use SplitStringPiece followed by JoinString so that no
	// copies of those strings are created until the final join operation.
	//
	// Use StrCat (in base/strings/strcat.h) if you don't need a separator.
	BASE_EXPORT std::string JoinString(const std::vector<std::string>& parts, std::string_view separator);
	BASE_EXPORT std::wstring JoinString(const std::vector<std::wstring>& parts, std::wstring_view separator);
	BASE_EXPORT std::string JoinString(const std::vector<std::string_view>& parts, std::string_view separator);
	BASE_EXPORT std::wstring JoinString(const std::vector<std::wstring_view>& parts, std::wstring_view separator);
	// Explicit initializer_list overloads are required to break ambiguity when used
	// with a literal initializer list (otherwise the compiler would not be able to
	// decide between the string and std::string_view overloads).
	BASE_EXPORT std::string JoinString(std::initializer_list<std::string_view> parts, std::string_view separator);
	BASE_EXPORT std::wstring JoinString(std::initializer_list<std::wstring_view> parts, std::wstring_view separator);

	// Replace $1-$2-$3..$9 in the format string with values from |subst|.
	// Additionally, any number of consecutive '$' characters is replaced by that
	// number less one. Eg $$->$, $$$->$$, etc. The offsets parameter here can be
	// NULL. This only allows you to use up to nine replacements.
	BASE_EXPORT std::wstring ReplaceStringPlaceholders(const std::wstring& format_string, const std::vector<std::wstring>& subst,
		std::vector<size_t>* offsets);

	BASE_EXPORT std::string ReplaceStringPlaceholders(std::string_view format_string, const std::vector<std::string>& subst,
		std::vector<size_t>* offsets);

	// Single-string shortcut for ReplaceStringHolders. |offset| may be NULL.
	BASE_EXPORT std::wstring ReplaceStringPlaceholders(const std::wstring& format_string, const std::wstring& a, size_t* offset);


	// Hashing ---------------------------------------------------------------------

	// We provide appropriate hash functions so StringPiece and StringPiece16 can
	// be used as keys in hash sets and maps.

	// This hash function is copied from base/strings/string16.h. We don't use the
	// ones already defined for string and string16 directly because it would
	// require the string constructors to be called, which we don't want.

	template <typename StringPieceType>
	struct StringPieceHashImpl {
		std::size_t operator()(StringPieceType sp) const {
			std::size_t result = 0;
			for (auto c : sp)
				result = (result * 131) + c;
			return result;
		}
	};

	using StringPieceHash = StringPieceHashImpl<std::string_view>;
	using WStringPieceHash = StringPieceHashImpl<std::wstring_view>;
}  // namespace base

#if defined(OS_WIN)
#include "strings/string_util_win.h"
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "strings/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif