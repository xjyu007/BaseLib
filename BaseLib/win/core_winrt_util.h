// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <hstring.h>
#include <inspectable.h>
#include <roapi.h>
#include <windef.h>

#include "base_export.h"
#include "scoped_hstring.h"

namespace base {
	namespace win {

		// Provides access to Core WinRT functions which may not be available on
		// Windows 7. Loads functions dynamically at runtime to prevent library
		// dependencies.

		BASE_EXPORT bool ResolveCoreWinRTDelayload();

		// The following stubs are provided for when component build is enabled, in
		// order to avoid the propagation of delay-loading CoreWinRT to other modules.

		BASE_EXPORT HRESULT RoInitialize(RO_INIT_TYPE init_type);

		BASE_EXPORT void RoUninitialize();

		BASE_EXPORT HRESULT RoGetActivationFactory(HSTRING class_id,
			const IID& iid,
			void** out_factory);

		BASE_EXPORT HRESULT RoActivateInstance(HSTRING class_id,
			IInspectable** instance);                                   

		// Retrieves an activation factory for the type specified.
		template <typename InterfaceType, wchar_t const* runtime_class_id>
		HRESULT GetActivationFactory(InterfaceType** factory) {
			const auto class_id_hstring = ScopedHString::Create(runtime_class_id);
			if (!class_id_hstring.is_valid())
				return E_FAIL;

			return base::win::RoGetActivationFactory(class_id_hstring.get(),
				IID_PPV_ARGS(factory));
		}

	}  // namespace win
}  // namespace base
