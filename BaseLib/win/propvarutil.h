// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#pragma once

// Check no prior poisonous defines were made.
#include "win/windows_defines.inc"
// Undefine before windows header will make the poisonous defines
#include "win/windows_undefines.inc"

#include <propvarutil.h>

// Undefine the poisonous defines
#include "win/windows_undefines.inc"
// Check no poisonous defines follow this include
#include "win/windows_defines.inc"
