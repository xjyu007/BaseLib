// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "file_version_info_win.h"

#include <Windows.h>

#include "files/file_path.h"
#include "logging.h"
#include "memory/ptr_util.h"
#include "stl_util.h"
#include "strings/string_util.h"
#include "threading/scoped_blocking_call.h"
#include "win/resource_util.h"

using base::FilePath;

#pragma comment(lib, "Version.lib")

namespace {

	struct LanguageAndCodePage {
		WORD language;
		WORD code_page;
	};

	// Returns the \\VarFileInfo\\Translation value extracted from the
	// VS_VERSION_INFO resource in |data|.
	LanguageAndCodePage* GetTranslate(const void* data) {
		LanguageAndCodePage* translate = nullptr;
		UINT length;
		if (::VerQueryValue(data, L"\\VarFileInfo\\Translation", 
							reinterpret_cast<void**>(&translate), &length)) {
			return translate;
		}
		return nullptr;
	}

	VS_FIXEDFILEINFO* GetVsFixedFileInfo(const void* data) {
		VS_FIXEDFILEINFO* fixed_file_info = nullptr;
		UINT length;
		if (::VerQueryValue(data, L"\\", reinterpret_cast<void**>(&fixed_file_info), 
							&length)) {
			return fixed_file_info;
		}
		return nullptr;
	}

}  // namespace

FileVersionInfoWin::~FileVersionInfoWin() = default;

// static
std::unique_ptr<FileVersionInfo>
FileVersionInfo::CreateFileVersionInfoForModule(HMODULE module) {
	void* data;
	size_t version_info_length;
	const bool has_version_resource = base::win::GetResourceFromModule(
		module, VS_VERSION_INFO, RT_VERSION, &data, &version_info_length);
	if (!has_version_resource)
		return nullptr;

	const LanguageAndCodePage* translate = GetTranslate(data);
	if (!translate)
		return nullptr;

	return base::WrapUnique(
		new FileVersionInfoWin(data, translate->language, translate->code_page));
}

// static
std::unique_ptr<FileVersionInfo> FileVersionInfo::CreateFileVersionInfo(
		const FilePath& file_path) {
	return FileVersionInfoWin::CreateFileVersionInfoWin(file_path);
}

// static
std::unique_ptr<FileVersionInfoWin>
FileVersionInfoWin::CreateFileVersionInfoWin(const FilePath& file_path) {
	base::ScopedBlockingCall scoped_blocking_call(FROM_HERE, 
												  base::BlockingType::MAY_BLOCK);

	DWORD dummy;
	const auto path = base::as_wcstr(file_path.value());
	const auto length = ::GetFileVersionInfoSize(path, &dummy);
	if (length == 0)
		return nullptr;

	std::vector<uint8_t> data(length, 0);

	if (!::GetFileVersionInfo(path, dummy, length, data.data()))
		return nullptr;

	const LanguageAndCodePage* translate = GetTranslate(data.data());
	if (!translate)
		return nullptr;

	return base::WrapUnique(new FileVersionInfoWin(
		std::move(data), translate->language, translate->code_page));
}

std::wstring FileVersionInfoWin::company_name() {
	return GetStringValue(L"CompanyName");
}

std::wstring FileVersionInfoWin::company_short_name() {
	return GetStringValue(L"CompanyShortName");
}

std::wstring FileVersionInfoWin::internal_name() {
	return GetStringValue(L"InternalName");
}

std::wstring FileVersionInfoWin::product_name() {
	return GetStringValue(L"ProductName");
}

std::wstring FileVersionInfoWin::product_short_name() {
	return GetStringValue(L"ProductShortName");
}

std::wstring FileVersionInfoWin::product_version() {
	return GetStringValue(L"ProductVersion");
}

std::wstring FileVersionInfoWin::file_description() {
	return GetStringValue(L"FileDescription");
}

std::wstring FileVersionInfoWin::file_version() {
	return GetStringValue(L"FileVersion");
}

std::wstring FileVersionInfoWin::original_filename() {
	return GetStringValue(L"OriginalFilename");
}

std::wstring FileVersionInfoWin::special_build() {
	return GetStringValue(L"SpecialBuild");
}

bool FileVersionInfoWin::GetValue(const wchar_t* name, 
								  std::wstring* value_str) const {
	WORD lang_codepage[8];
	size_t i = 0;
	// Use the language and codepage from the DLL.
	lang_codepage[i++] = language_;
	lang_codepage[i++] = code_page_;
	// Use the default language and codepage from the DLL.
	lang_codepage[i++] = ::GetUserDefaultLangID();
	lang_codepage[i++] = code_page_;
	// Use the language from the DLL and Latin codepage (most common).
	lang_codepage[i++] = language_;
	lang_codepage[i++] = 1252;
	// Use the default language and Latin codepage (most common).
	lang_codepage[i++] = ::GetUserDefaultLangID();
	lang_codepage[i] = 1252;

	i = 0;
	while (i < base::size(lang_codepage)) {
		wchar_t sub_block[MAX_PATH];
		const auto language = lang_codepage[i++];
		const auto code_page = lang_codepage[i++];
		_snwprintf_s(sub_block, MAX_PATH, MAX_PATH,	L"\\StringFileInfo\\%04x%04x\\%ls", language, code_page, base::as_wcstr(name));
		LPVOID value = nullptr;
		uint32_t size;
		const auto r = ::VerQueryValue(data_, sub_block, &value, &size);
		if (r && value) {
			value_str->assign(static_cast<wchar_t*>(value));
			return true;
		}
	}
	return false;
}

std::wstring FileVersionInfoWin::GetStringValue(const wchar_t* name) const {
	std::wstring str;
	if (GetValue(name, &str))
		return str;
	return std::wstring();
}

FileVersionInfoWin::FileVersionInfoWin(std::vector<uint8_t>&& data, 
									   WORD language, 
									   WORD code_page)
	: owned_data_(std::move(data)),
	data_(owned_data_.data()),
	language_(language),
	code_page_(code_page),
	fixed_file_info_(GetVsFixedFileInfo(data_)) {
	DCHECK(!owned_data_.empty());
}

FileVersionInfoWin::FileVersionInfoWin(void* data, 
									   WORD language, 
									   WORD code_page)
	: data_(data),
	language_(language),
	code_page_(code_page),
	fixed_file_info_(GetVsFixedFileInfo(data)) {
	DCHECK(data_);
}