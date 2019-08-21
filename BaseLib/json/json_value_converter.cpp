// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "json/json_value_converter.h"

#include "strings/utf_string_conversions.h"

namespace base::internal {

	bool BasicValueConverter<int>::Convert(
			const Value& value, int* field) const {
		if (!value.is_int())
			return false;
		if (field)
			* field = value.GetInt();
		return true;
	}

	bool BasicValueConverter<std::string>::Convert(
			const Value& value, std::string* field) const {
		if (!value.is_string())
			return false;
		if (field)
			* field = value.GetString();
		return true;
	}

	bool BasicValueConverter<std::wstring>::Convert(
			const Value& value, std::wstring* field) const {
		if (!value.is_string())
			return false;
		if (field)
			* field = base::UTF8ToWide(value.GetString());
		return true;
	}

	bool BasicValueConverter<double>::Convert(
			const Value& value, double* field) const {
		if (!value.is_double() && !value.is_int())
			return false;
		if (field)
			* field = value.GetDouble();
		return true;
	}

	bool BasicValueConverter<bool>::Convert(
			const Value& value, bool* field) const {
		if (!value.is_bool())
			return false;
		if (field)
			* field = value.GetBool();
		return true;
	}
} // namespace base

