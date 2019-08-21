// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "debug_/crash_logging.h"

namespace base::debug {
	namespace {
		CrashKeyImplementation* g_crash_key_impl = nullptr;
	}  // namespace

	CrashKeyString* AllocateCrashKeyString(const char name[], CrashKeySize value_length) {
		if (!g_crash_key_impl)
			return nullptr;

		return g_crash_key_impl->Allocate(name, value_length);
	}

	void SetCrashKeyString(CrashKeyString* crash_key, std::string_view value) {
		if (!g_crash_key_impl || !crash_key)
			return;

		g_crash_key_impl->Set(crash_key, value);
	}

	void ClearCrashKeyString(CrashKeyString* crash_key) {
		if (!g_crash_key_impl || !crash_key)
			return;

		g_crash_key_impl->Clear(crash_key);
	}

	ScopedCrashKeyString::ScopedCrashKeyString(CrashKeyString* crash_key, std::string_view value) : crash_key_(crash_key) {
		SetCrashKeyString(crash_key_, value);
	}

	ScopedCrashKeyString::~ScopedCrashKeyString() {
		ClearCrashKeyString(crash_key_);
	}

	void SetCrashKeyImplementation(std::unique_ptr<CrashKeyImplementation> impl) {
		delete g_crash_key_impl;
		g_crash_key_impl = impl.release();
	}
} // namespace base
