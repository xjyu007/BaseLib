// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "i18n/rtl.h"

#include <cstdint>

#include <algorithm>

#include "command_line.h"
#include "files/file_path.h"
#include "i18n/base_i18n_switches.h"
#include "logging.h"
#include "stl_util.h"
#include "strings/string_split.h"
#include "strings/string_util.h"
#include "strings/sys_string_conversions.h"
#include "build_config.h"
#include "unicode/locid.h"
#include "unicode/uchar.h"
#include "unicode/coll.h"

namespace {

	// Extract language, country and variant, but ignore keywords.  For example,
	// en-US, ca@valencia, ca-ES@valencia.
	std::string GetLocaleString(const icu::Locale& locale) {
		const char* language = locale.getLanguage();
		const char* country = locale.getCountry();
		const char* variant = locale.getVariant();

		std::string result = (language != nullptr && *language != '\0') ? language : "und";

		if (country != nullptr && *country != '\0') {
			result += '-';
			result += country;
		}

		if (variant != nullptr && *variant != '\0')
			result += '@' + base::ToLowerASCII(variant);

		return result;
	}

	// Returns LEFT_TO_RIGHT or RIGHT_TO_LEFT if |character| has strong
	// directionality, returns UNKNOWN_DIRECTION if it doesn't. Please refer to
	// http://unicode.org/reports/tr9/ for more information.
	base::i18n::TextDirection GetCharacterDirection(UChar32 character) {
		static bool has_switch = base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kForceTextDirection);
		if (has_switch) {
			base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
			std::string force_flag =
				command_line->GetSwitchValueASCII(switches::kForceTextDirection);

			if (force_flag == switches::kForceDirectionRTL)
				return base::i18n::RIGHT_TO_LEFT;
			if (force_flag == switches::kForceDirectionLTR)
				return base::i18n::LEFT_TO_RIGHT;
		}
		// Now that we have the character, we use ICU in order to query for the
		// appropriate Unicode BiDi character type.
		int32_t property = u_getIntPropertyValue(character, UCHAR_BIDI_CLASS);
		switch (property) {
		case U_RIGHT_TO_LEFT:
		case U_RIGHT_TO_LEFT_ARABIC:
		case U_RIGHT_TO_LEFT_EMBEDDING:
		case U_RIGHT_TO_LEFT_OVERRIDE:
			return base::i18n::RIGHT_TO_LEFT;
		case U_LEFT_TO_RIGHT:
		case U_LEFT_TO_RIGHT_EMBEDDING:
		case U_LEFT_TO_RIGHT_OVERRIDE:
			return base::i18n::LEFT_TO_RIGHT;
		}
		return base::i18n::UNKNOWN_DIRECTION;
	}

}  // namespace

namespace base::i18n
{

		// Represents the locale-specific ICU text direction.
		static TextDirection g_icu_text_direction = UNKNOWN_DIRECTION;

		// Convert the ICU default locale to a string.
		std::string GetConfiguredLocale() {
			return GetLocaleString(icu::Locale::getDefault());
		}

		// Convert the ICU canonicalized locale to a string.
		std::string GetCanonicalLocale(const std::string& locale) {
			return GetLocaleString(icu::Locale::createCanonical(locale.c_str()));
		}

		// Convert Chrome locale name to ICU locale name
		std::string ICULocaleName(const std::string& locale_string) {
			// If not Spanish, just return it.
			if (locale_string.substr(0, 2) != "es")
				return locale_string;
			// Expand es to es-ES.
			if (LowerCaseEqualsASCII(locale_string, "es"))
				return "es-ES";
			// Map es-419 (Latin American Spanish) to es-FOO depending on the system
			// locale.  If it's es-RR other than es-ES, map to es-RR. Otherwise, map
			// to es-MX (the most populous in Spanish-speaking Latin America).
			if (LowerCaseEqualsASCII(locale_string, "es-419")) {
				const icu::Locale& locale = icu::Locale::getDefault();
				std::string language = locale.getLanguage();
				const char* country = locale.getCountry();
				if (LowerCaseEqualsASCII(language, "es") &&
					!LowerCaseEqualsASCII(country, "es")) {
					language += '-';
					language += country;
					return language;
				}
				return "es-MX";
			}
			// Currently, Chrome has only "es" and "es-419", but later we may have
			// more specific "es-RR".
			return locale_string;
		}

		void SetICUDefaultLocale(const std::string& locale_string) {
			icu::Locale locale(ICULocaleName(locale_string).c_str());
			UErrorCode error_code = U_ZERO_ERROR;
			const char* lang = locale.getLanguage();
			if (lang != nullptr && *lang != '\0') {
				icu::Locale::setDefault(locale, error_code);
			}
			else {
				LOG(ERROR) << "Failed to set the ICU default locale to " << locale_string
					<< ". Falling back to en-US.";
				icu::Locale::setDefault(icu::Locale::getUS(), error_code);
			}
			g_icu_text_direction = UNKNOWN_DIRECTION;
		}

		bool IsRTL() {
			return ICUIsRTL();
		}

		void SetRTLForTesting(bool rtl) {
			SetICUDefaultLocale(rtl ? "he" : "en");
			DCHECK_EQ(rtl, IsRTL());
		}

		bool ICUIsRTL() {
			if (g_icu_text_direction == UNKNOWN_DIRECTION) {
				const icu::Locale& locale = icu::Locale::getDefault();
				g_icu_text_direction = GetTextDirectionForLocaleInStartUp(locale.getName());
			}
			return g_icu_text_direction == RIGHT_TO_LEFT;
		}

		TextDirection GetForcedTextDirection() {
			// On iOS, check for RTL forcing.
			auto command_line = CommandLine::ForCurrentProcess();
			if (command_line->HasSwitch(switches::kForceUIDirection)) {
				std::string force_flag =
					command_line->GetSwitchValueASCII(switches::kForceUIDirection);

				if (force_flag == switches::kForceDirectionLTR)
					return base::i18n::LEFT_TO_RIGHT;

				if (force_flag == switches::kForceDirectionRTL)
					return base::i18n::RIGHT_TO_LEFT;
			}

			return UNKNOWN_DIRECTION;
		}

		TextDirection GetTextDirectionForLocaleInStartUp(const char* locale_name) {
			// Check for direction forcing.
			TextDirection forced_direction = GetForcedTextDirection();
			if (forced_direction != UNKNOWN_DIRECTION)
				return forced_direction;

			// This list needs to be updated in alphabetical order if we add more RTL
			// locales.
			static const char kRTLLanguageCodes[][3] = { "ar", "fa", "he", "iw", "ur" };
			std::vector<std::string_view> locale_split =
				SplitStringPiece(locale_name, "-_", KEEP_WHITESPACE, SPLIT_WANT_ALL);
			const std::string_view& language_code = locale_split[0];
			if (std::binary_search(kRTLLanguageCodes,
				kRTLLanguageCodes + base::size(kRTLLanguageCodes),
				language_code))
				return RIGHT_TO_LEFT;
			return LEFT_TO_RIGHT;
		}

		TextDirection GetTextDirectionForLocale(const char* locale_name) {
			// Check for direction forcing.
			TextDirection forced_direction = GetForcedTextDirection();
			if (forced_direction != UNKNOWN_DIRECTION)
				return forced_direction;

			UErrorCode status = U_ZERO_ERROR;
			ULayoutType layout_dir = uloc_getCharacterOrientation(locale_name, &status);
			DCHECK(U_SUCCESS(status));
			// Treat anything other than RTL as LTR.
			return (layout_dir != ULOC_LAYOUT_RTL) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;
		}

		TextDirection GetFirstStrongCharacterDirection(const std::wstring& text) {
			const UChar* string = reinterpret_cast<const UChar*>(text.c_str());
			size_t length = text.length();
			size_t position = 0;
			while (position < length) {
				UChar32 character;
				size_t next_position = position;
				U16_NEXT(string, next_position, length, character);
				TextDirection direction = GetCharacterDirection(character);
				if (direction != UNKNOWN_DIRECTION)
					return direction;
				position = next_position;
			}
			return LEFT_TO_RIGHT;
		}

		TextDirection GetLastStrongCharacterDirection(const std::wstring& text) {
			const UChar* string = reinterpret_cast<const UChar*>(text.c_str());
			size_t position = text.length();
			while (position > 0) {
				UChar32 character;
				size_t prev_position = position;
				U16_PREV(string, 0, prev_position, character);
				TextDirection direction = GetCharacterDirection(character);
				if (direction != UNKNOWN_DIRECTION)
					return direction;
				position = prev_position;
			}
			return LEFT_TO_RIGHT;
		}

		TextDirection GetStringDirection(const std::wstring& text) {
			const UChar* string = reinterpret_cast<const UChar*>(text.c_str());
			size_t length = text.length();
			size_t position = 0;

			TextDirection result(UNKNOWN_DIRECTION);
			while (position < length) {
				UChar32 character;
				size_t next_position = position;
				U16_NEXT(string, next_position, length, character);
				TextDirection direction = GetCharacterDirection(character);
				if (direction != UNKNOWN_DIRECTION) {
					if (result != UNKNOWN_DIRECTION && result != direction)
						return UNKNOWN_DIRECTION;
					result = direction;
				}
				position = next_position;
			}

			// Handle the case of a string not containing any strong directionality
			// characters defaulting to LEFT_TO_RIGHT.
			if (result == UNKNOWN_DIRECTION)
				return LEFT_TO_RIGHT;

			return result;
		}

		bool AdjustStringForLocaleDirection(std::wstring* text) {
			if (!IsRTL() || text->empty())
				return false;

			// Marking the string as LTR if the locale is RTL and the string does not
			// contain strong RTL characters. Otherwise, mark the string as RTL.
			bool has_rtl_chars = StringContainsStrongRTLChars(*text);
			if (!has_rtl_chars)
				WrapStringWithLTRFormatting(text);
			else
				WrapStringWithRTLFormatting(text);

			return true;
		}

		bool UnadjustStringForLocaleDirection(std::wstring* text) {
			if (!IsRTL() || text->empty())
				return false;

			*text = StripWrappingBidiControlCharacters(*text);
			return true;
		}

		void EnsureTerminatedDirectionalFormatting(std::wstring* text) {
			int count = 0;
			for (auto c : *text) {
				if (c == kLeftToRightEmbeddingMark || c == kRightToLeftEmbeddingMark ||
					c == kLeftToRightOverride || c == kRightToLeftOverride) {
					++count;
				}
				else if (c == kPopDirectionalFormatting && count > 0) {
					--count;
				}
			}
			for (int j = 0; j < count; j++)
				text->push_back(kPopDirectionalFormatting);
		}

		void SanitizeUserSuppliedString(std::wstring* text) {
			EnsureTerminatedDirectionalFormatting(text);
			AdjustStringForLocaleDirection(text);
		}

		bool StringContainsStrongRTLChars(const std::wstring& text) {
			const UChar* string = reinterpret_cast<const UChar*>(text.c_str());
			size_t length = text.length();
			size_t position = 0;
			while (position < length) {
				UChar32 character;
				size_t next_position = position;
				U16_NEXT(string, next_position, length, character);

				// Now that we have the character, we use ICU in order to query for the
				// appropriate Unicode BiDi character type.
				int32_t property = u_getIntPropertyValue(character, UCHAR_BIDI_CLASS);
				if ((property == U_RIGHT_TO_LEFT) || (property == U_RIGHT_TO_LEFT_ARABIC))
					return true;

				position = next_position;
			}

			return false;
		}

		void WrapStringWithLTRFormatting(std::wstring* text) {
			if (text->empty())
				return;

			// Inserting an LRE (Left-To-Right Embedding) mark as the first character.
			text->insert(static_cast<size_t>(0), static_cast<size_t>(1),
				kLeftToRightEmbeddingMark);

			// Inserting a PDF (Pop Directional Formatting) mark as the last character.
			text->push_back(kPopDirectionalFormatting);
		}

		void WrapStringWithRTLFormatting(std::wstring* text) {
			if (text->empty())
				return;

			// Inserting an RLE (Right-To-Left Embedding) mark as the first character.
			text->insert(static_cast<size_t>(0), static_cast<size_t>(1),
				kRightToLeftEmbeddingMark);

			// Inserting a PDF (Pop Directional Formatting) mark as the last character.
			text->push_back(kPopDirectionalFormatting);
		}

		void WrapPathWithLTRFormatting(const FilePath& path,
			std::wstring* rtl_safe_path) {
			// Wrap the overall path with LRE-PDF pair which essentialy marks the
			// string as a Left-To-Right string.
			// Inserting an LRE (Left-To-Right Embedding) mark as the first character.
			rtl_safe_path->push_back(kLeftToRightEmbeddingMark);
			rtl_safe_path->append(path.value());
			// Inserting a PDF (Pop Directional Formatting) mark as the last character.
			rtl_safe_path->push_back(kPopDirectionalFormatting);
		}

		std::wstring GetDisplayStringInLTRDirectionality(const std::wstring& text) {
			// Always wrap the string in RTL UI (it may be appended to RTL string).
			// Also wrap strings with an RTL first strong character direction in LTR UI.
			if (IsRTL() || GetFirstStrongCharacterDirection(text) == RIGHT_TO_LEFT) {
				std::wstring text_mutable(text);
				WrapStringWithLTRFormatting(&text_mutable);
				return text_mutable;
			}
			return text;
		}

		std::wstring StripWrappingBidiControlCharacters(const std::wstring& text) {
			if (text.empty())
				return text;
			size_t begin_index = 0;
			wchar_t begin = text[begin_index];
			if (begin == kLeftToRightEmbeddingMark ||
				begin == kRightToLeftEmbeddingMark ||
				begin == kLeftToRightOverride ||
				begin == kRightToLeftOverride)
				++begin_index;
			size_t end_index = text.length() - 1;
			if (text[end_index] == kPopDirectionalFormatting)
				--end_index;
			return text.substr(begin_index, end_index - begin_index + 1);
		}
} // namespace base
