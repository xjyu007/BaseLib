// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unknwn.h>

#include "atomic_ref_count.h"
#include "base_export.h"
#include "compiler_specific.h"

namespace base {
	namespace win {

		// IUnknown implementation for other classes to derive from.
		class BASE_EXPORT IUnknownImpl : public IUnknown {
		public:
			IUnknownImpl();

			ULONG STDMETHODCALLTYPE AddRef() override;
			ULONG STDMETHODCALLTYPE Release() override;

			// Subclasses should extend this to return any interfaces they provide.
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;

		protected:
			virtual ~IUnknownImpl();

		private:
			AtomicRefCount ref_count_;
		};

	}  // namespace win
}  // namespace base
