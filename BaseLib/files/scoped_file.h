#pragma once

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <memory>

namespace base {

	namespace internal {
		// Functor for |ScopedFILE| (below).
		struct ScopedFILECloser {
			void operator()(FILE* x) const {
				if (x)
					fclose(x);
			}
		};

	}  // namespace internal

	// -----------------------------------------------------------------------------
	// Automatically closes |FILE*|s.
	typedef std::unique_ptr<FILE, internal::ScopedFILECloser> ScopedFILE;

}  // namespace base
