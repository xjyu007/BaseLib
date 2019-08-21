// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unknwn.h>

#include <memory>

#include "win/iunknown_impl.h"

namespace base {
	namespace win {

		// A simple implementation of IEnumVARIANT.
		class BASE_EXPORT EnumVariant : public IEnumVARIANT, public IUnknownImpl {
		public:
			// The constructor allocates an array of size |count|. Then use
			// ItemAt to set the value of each item in the array to initialize it.
			explicit EnumVariant(unsigned long count);

			// Returns a mutable pointer to the item at position |index|.
			VARIANT* ItemAt(unsigned long index) const;

			// IUnknown.
			ULONG STDMETHODCALLTYPE AddRef() override;
			ULONG STDMETHODCALLTYPE Release() override;
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;

			// IEnumVARIANT.
			STDMETHODIMP Next(ULONG requested_count, VARIANT* out_elements, ULONG* out_elements_received) override;
			STDMETHODIMP Skip(ULONG skip_count) override;
			STDMETHODIMP Reset() override;
			STDMETHODIMP Clone(IEnumVARIANT** out_cloned_object) override;

		private:
			~EnumVariant() override;

			std::unique_ptr<VARIANT[]> items_;
			unsigned long count_;
			unsigned long current_index_;
		};

	}  // namespace win
}  // namespace base
