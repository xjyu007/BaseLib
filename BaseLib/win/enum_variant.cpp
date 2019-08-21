// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win/enum_variant.h"

#include <algorithm>

#include "logging.h"

namespace base::win {

	EnumVariant::EnumVariant(unsigned long count)
		: items_(new VARIANT[count]),
		count_(count),
		current_index_(0) {}

	EnumVariant::~EnumVariant() = default;

	VARIANT* EnumVariant::ItemAt(unsigned long index) const {
		DCHECK(index < count_);
		return &items_[index];
	}

	ULONG STDMETHODCALLTYPE EnumVariant::AddRef() {
		return IUnknownImpl::AddRef();
	}

	ULONG STDMETHODCALLTYPE EnumVariant::Release() {
		return IUnknownImpl::Release();
	}

	STDMETHODIMP EnumVariant::QueryInterface(REFIID riid, void** ppv) {
		if (riid == IID_IEnumVARIANT) {
			*ppv = static_cast<IEnumVARIANT*>(this);
			AddRef();
			return S_OK;
		}

		return IUnknownImpl::QueryInterface(riid, ppv);
	}

	STDMETHODIMP EnumVariant::Next(ULONG requested_count, VARIANT* out_elements, ULONG* out_elements_received) {
#undef min
		const auto count = std::min(requested_count, count_ - current_index_);
		for (unsigned long i = 0; i < count; ++i)
			out_elements[i] = items_[current_index_ + i];
		current_index_ += count;
		*out_elements_received = count;

		return (count == requested_count ? S_OK : S_FALSE);
	}

	STDMETHODIMP EnumVariant::Skip(ULONG skip_count) {
		auto count = skip_count;
		if (current_index_ + count > count_)
			count = count_ - current_index_;

		current_index_ += count;
		return (count == skip_count ? S_OK : S_FALSE);
	}

	STDMETHODIMP EnumVariant::Reset() {
		current_index_ = 0;
		return S_OK;
	}

	STDMETHODIMP EnumVariant::Clone(IEnumVARIANT** out_cloned_object) {
		auto other = new EnumVariant(count_);
		if (count_ > 0)
			memcpy(other->ItemAt(0), &items_[0], count_ * sizeof(VARIANT));
		other->Skip(current_index_);
		other->AddRef();
		*out_cloned_object = other;
		return S_OK;
	}
} // namespace base
