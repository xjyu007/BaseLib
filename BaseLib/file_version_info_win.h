// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Windows.h>

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "base_export.h"
#include "file_version_info.h"
#include "macros.h"

struct tagVS_FIXEDFILEINFO;
typedef tagVS_FIXEDFILEINFO VS_FIXEDFILEINFO;

class BASE_EXPORT FileVersionInfoWin : public FileVersionInfo {
public:
	~FileVersionInfoWin() override;

	// Accessors to the different version properties.
	// Returns an empty string if the property is not found.
	std::wstring company_name() override;
	std::wstring company_short_name() override;
	std::wstring product_name() override;
	std::wstring product_short_name() override;
	std::wstring internal_name() override;
	std::wstring product_version() override;
	std::wstring special_build() override;
	std::wstring original_filename() override;
	std::wstring file_description() override;
	std::wstring file_version() override;

	// Lets you access other properties not covered above.
	bool GetValue(const wchar_t* name, std::wstring* value) const;

	// Similar to GetValue but returns a string16 (empty string if the property
	// does not exist).
	std::wstring GetStringValue(const wchar_t* name) const;

	// Get the fixed file info if it exists. Otherwise NULL
	[[nodiscard]] const VS_FIXEDFILEINFO* fixed_file_info() const { return fixed_file_info_; }

	// Behaves like CreateFileVersionInfo, but returns a FileVersionInfoWin.
	static std::unique_ptr<FileVersionInfoWin> CreateFileVersionInfoWin(
		const base::FilePath& file_path);

private:
	friend FileVersionInfo;

	// |data| is a VS_VERSION_INFO resource. |language| and |code_page| are
	// extracted from the \VarFileInfo\Translation value of |data|.
	FileVersionInfoWin(std::vector<uint8_t>&& data,
					   WORD language,
					   WORD code_page);
	FileVersionInfoWin(void* data, WORD language, WORD code_page);

	const std::vector<uint8_t> owned_data_;
	const void* const data_;
	const WORD language_;
	const WORD code_page_;

	// This is a pointer into |data_| if it exists. Otherwise nullptr.
	const VS_FIXEDFILEINFO* const fixed_file_info_;

	DISALLOW_COPY_AND_ASSIGN(FileVersionInfoWin);
};
