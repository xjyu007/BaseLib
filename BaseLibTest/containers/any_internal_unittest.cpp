// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "containers/any_internal.h"

namespace base::internal
{

		namespace {
			struct OutOfLineStruct {
				void* one;
				void* two;
			};
		}  // namespace

		TEST(AnyInternalTest, InlineOrOutlineStorage) {
			static_assert(AnyInternal::InlineStorageHelper<int>::kUseInlineStorage,
				"int should be stored inline");
			static_assert(AnyInternal::InlineStorageHelper<int*>::kUseInlineStorage,
				"int* should be stored inline");
			static_assert(
				AnyInternal::InlineStorageHelper<std::unique_ptr<int>>::kUseInlineStorage,
				"std::unique_ptr<int> should be stored inline");
			static_assert(
				!AnyInternal::InlineStorageHelper<OutOfLineStruct>::kUseInlineStorage,
				"A struct with two pointers should be stored out of line");
		}
} // namespace base
