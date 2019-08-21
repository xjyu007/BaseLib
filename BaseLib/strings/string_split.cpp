// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "strings/string_split.h"
#include "logging.h"
#include "strings/string_util.h"

namespace base {
	namespace {

		// PieceToOutputType converts a std::string_view as needed to a given output type,
		// which is either the same type of std::string_view (a NOP) or the corresponding
		// non-piece string type.
		//
		// The default converter is a NOP, it works when the OutputType is the
		// correct std::string_view.
		template<typename Str, typename OutputType>
		OutputType PieceToOutputType(std::basic_string_view<Str> piece) {
			return piece;
		}
		template<>  // Convert std::string_view to std::string
		std::string PieceToOutputType<char, std::string>(std::string_view piece) {
			return std::string(piece.data());
		}
		template<>  // Convert std::wstring_view to std::wstring.
		std::wstring PieceToOutputType<wchar_t, std::wstring>(std::wstring_view piece) {
			return std::wstring(piece.data());
		}

		// Returns either the ASCII or UTF-16 whitespace.
		template<typename Str> std::basic_string_view<Str> WhitespaceForType();
		template<> std::wstring_view WhitespaceForType<wchar_t>() {
			return kWhitespaceUTF16;
		}
		template<> std::string_view WhitespaceForType<char>() {
			return kWhitespaceASCII;
		}
		// Optimize the single-character case to call find() on the string instead,
		// since this is the common case and can be made faster. This could have been
		// done with template specialization too, but would have been less clear.
		//
		// There is no corresponding FindFirstNotOf because std::string_view already
		// implements these different versions that do the optimized searching.
		size_t FindFirstOf(std::string_view piece, char c, size_t pos) {
			return piece.find(c, pos);
		}
		size_t FindFirstOf(std::wstring_view piece, wchar_t c, size_t pos) {
			return piece.find(c, pos);
		}
		size_t FindFirstOf(std::string_view piece, std::string_view one_of, size_t pos) {
			return piece.find_first_of(one_of, pos);
		}
		size_t FindFirstOf(std::wstring_view piece, std::wstring_view one_of, size_t pos) {
			return piece.find_first_of(one_of, pos);
		}

		// General string splitter template. Can take 8- or 16-bit input, can produce
		// the corresponding string or std::string_view output, and can take single- or
		// multiple-character delimiters.
		//
		// DelimiterType is either a character (Str::value_type) or a string piece of
		// multiple characters (std::basic_string_view<Str>). std::string_view has a version of
		// find for both of these cases, and the single-character version is the most
		// common and can be implemented faster, which is why this is a template.
		template<typename Str, typename OutputStringType, typename DelimiterType>
		std::vector<OutputStringType> SplitStringT(
			std::basic_string_view<Str> str,
			DelimiterType delimiter,
			WhitespaceHandling whitespace,
			SplitResult result_type) {
			std::vector<OutputStringType> result;
			if (str.empty())
				return result;

			size_t start = 0;
			while (start != std::basic_string_view<Str>::npos) {
				size_t end = FindFirstOf(str, delimiter, start);

				std::basic_string_view<Str> piece;
				if (end == std::basic_string_view<Str>::npos) {
					piece = str.substr(start);
					start = std::basic_string_view<Str>::npos;
				}
				else {
					piece = str.substr(start, end - start);
					start = end + 1;
				}

				if (whitespace == TRIM_WHITESPACE)
					piece = TrimString(piece, WhitespaceForType<Str>(), TRIM_ALL);

				if (result_type == SPLIT_WANT_ALL || !piece.empty())
					result.push_back(PieceToOutputType<Str, OutputStringType>(piece));
			}
			return result;
		}

		bool AppendStringKeyValue(std::string_view input,
			char delimiter,
			StringPairs* result) {
			// Always append a new item regardless of success (it might be empty). The
			// below code will copy the strings directly into the result pair.
			result->resize(result->size() + 1);
			auto& result_pair = result->back();

			// Find the delimiter.
			const auto end_key_pos = input.find_first_of(delimiter);
			if (end_key_pos == std::string::npos) {
				DVLOG(1) << "cannot find delimiter in: " << input;
				return false;    // No delimiter.
			}
			//input.substr(0, end_key_pos).CopyToString(&result_pair.first);
			result_pair.first = input.substr(0, end_key_pos);

			// Find the value string.
			const auto remains = input.substr(end_key_pos, input.size() - end_key_pos);
			const auto begin_value_pos = remains.find_first_not_of(delimiter);
			if (begin_value_pos == std::string_view::npos) {
				DVLOG(1) << "cannot parse value from input: " << input;
				return false;   // No value.
			}
			//remains.substr(begin_value_pos, remains.size() - begin_value_pos).CopyToString(&result_pair.second);
			result_pair.second = remains.substr(begin_value_pos, remains.size() - begin_value_pos);

			return true;
		}

		template <typename Str, typename OutputStringType>
		void SplitStringUsingSubstrT(std::basic_string_view<Str> input,
			std::basic_string_view<Str> delimiter,
			WhitespaceHandling whitespace,
			SplitResult result_type,
			std::vector<OutputStringType>* result) {
			using Piece = std::basic_string_view<Str>;
			using size_type = typename Piece::size_type;

			result->clear();
			for (size_type begin_index = 0, end_index = 0; end_index != Piece::npos;
				begin_index = end_index + delimiter.size()) {
				end_index = input.find(delimiter, begin_index);
				Piece term = end_index == Piece::npos
					? input.substr(begin_index)
					: input.substr(begin_index, end_index - begin_index);

				if (whitespace == TRIM_WHITESPACE)
					term = TrimString(term, WhitespaceForType<Str>(), TRIM_ALL);

				if (result_type == SPLIT_WANT_ALL || !term.empty())
					result->push_back(PieceToOutputType<Str, OutputStringType>(term));
			}
		}

	}  // namespace

	std::vector<std::string> SplitString(std::string_view input,
		std::string_view separators,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		if (separators.size() == 1) {
			return SplitStringT<char, std::string, char>(input, separators[0], whitespace, result_type);
		}
		return SplitStringT<char, std::string, std::string_view>(input, separators, whitespace, result_type);
	}

	std::vector<std::wstring> SplitString(std::wstring_view input,
		std::wstring_view separators,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		if (separators.size() == 1) {
			return SplitStringT<wchar_t, std::wstring, wchar_t>(
				input, separators[0], whitespace, result_type);
		}
		return SplitStringT<wchar_t, std::wstring, std::wstring_view>(
			input, separators, whitespace, result_type);
	}

	std::vector<std::string_view> SplitStringPiece(std::string_view input,
		std::string_view separators,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		if (separators.size() == 1) {
			return SplitStringT<char, std::string_view, char>(
				input, separators[0], whitespace, result_type);
		}
		return SplitStringT<char, std::string_view, std::string_view>(
			input, separators, whitespace, result_type);
	}

	std::vector<std::wstring_view> SplitStringPiece(std::wstring_view input,
		std::wstring_view separators,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		if (separators.size() == 1) {
			return SplitStringT<wchar_t, std::wstring_view, wchar_t>(
				input, separators[0], whitespace, result_type);
		}
		return SplitStringT<wchar_t, std::wstring_view, std::wstring_view>(
			input, separators, whitespace, result_type);
	}

	bool SplitStringIntoKeyValuePairs(std::string_view input,
		char key_value_delimiter,
		char key_value_pair_delimiter,
		StringPairs* key_value_pairs) {
		return SplitStringIntoKeyValuePairsUsingSubstr(
			input, key_value_delimiter, std::string_view(&key_value_pair_delimiter, 1),
			key_value_pairs);
	}

	bool SplitStringIntoKeyValuePairsUsingSubstr(
		std::string_view input,
		char key_value_delimiter,
		std::string_view key_value_pair_delimiter,
		StringPairs* key_value_pairs) {
		key_value_pairs->clear();

		std::vector<std::string_view> pairs = SplitStringPieceUsingSubstr(
			input, key_value_pair_delimiter, TRIM_WHITESPACE, SPLIT_WANT_NONEMPTY);
		key_value_pairs->reserve(pairs.size());

		bool success = true;
		for (const std::string_view& pair : pairs) {
			if (!AppendStringKeyValue(pair, key_value_delimiter, key_value_pairs)) {
				// Don't return here, to allow for pairs without associated
				// value or key; just record that the split failed.
				success = false;
			}
		}
		return success;
	}

	std::vector<std::wstring> SplitStringUsingSubstr(std::wstring_view input,
		std::wstring_view delimiter,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		std::vector<std::wstring> result;
		SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
		return result;
	}

	std::vector<std::string> SplitStringUsingSubstr(std::string_view input,
		std::string_view delimiter,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		std::vector<std::string> result;
		SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
		return result;
	}

	std::vector<std::wstring_view> SplitStringPieceUsingSubstr(
		std::wstring_view input,
		std::wstring_view delimiter,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		std::vector<std::wstring_view> result;
		SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
		return result;
	}

	std::vector<std::string_view> SplitStringPieceUsingSubstr(
		std::string_view input,
		std::string_view delimiter,
		WhitespaceHandling whitespace,
		SplitResult result_type) {
		std::vector<std::string_view> result;
		SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
		return result;
	}

}  // namespace base
