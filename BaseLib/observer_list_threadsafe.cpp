// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "observer_list_threadsafe.h"

namespace base::internal {

	LazyInstance<ThreadLocalPointer<
		const ObserverListThreadSafeBase::NotificationDataBase>>::Leaky
		ObserverListThreadSafeBase::tls_current_notification_ =
		LAZY_INSTANCE_INITIALIZER;
} // namespace base
