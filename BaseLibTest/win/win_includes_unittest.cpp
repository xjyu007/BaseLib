// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file ensures that these header files don't include Windows.h and can
// compile without including Windows.h. This helps to improve compile times.

#include "pch.h"
#include "atomicops.h"
#include "files/file_util.h"
#include "files/platform_file.h"
#include "process/process_handle.h"
#include "synchronization/condition_variable.h"
#include "synchronization/lock.h"
#include "threading/platform_thread.h"
#include "threading/thread_local_storage.h"
#include "win/registry.h"
#include "win/scoped_handle.h"
#include "win/win_util.h"

#ifdef _WINDOWS_
#error Windows.h was included inappropriately.
#endif

// Make sure windows.h can be included after windows_types.h
#include "win/windows_types.h"

#include <windows.h>

// Check that type sizes match.
static_assert(sizeof(CHROME_CONDITION_VARIABLE) == sizeof(CONDITION_VARIABLE), "Definition mismatch.");
static_assert(sizeof(CHROME_SRWLOCK) == sizeof(SRWLOCK), "Definition mismatch.");
