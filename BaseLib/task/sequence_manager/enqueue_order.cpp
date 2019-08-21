// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "task/sequence_manager/enqueue_order.h"

namespace base::sequence_manager::internal
{

			EnqueueOrder::Generator::Generator() : counter_(kFirst) {}

			EnqueueOrder::Generator::~Generator() = default;
} // namespace base
