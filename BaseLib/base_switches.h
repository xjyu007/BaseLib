#pragma once

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "base" command-line switches.

namespace switches {
	extern const char kDisableBestEffortTasks[];
	extern const char kDisableBreakpad[];
	extern const char kDisableFeatures[];
	extern const char kDisableLowEndDeviceMode[];
	extern const char kEnableCrashReporter[];
	extern const char kEnableFeatures[];
	extern const char kEnableLowEndDeviceMode[];
	extern const char kForceFieldTrials[];
	extern const char kFullMemoryCrashReport[];
	extern const char kLogBestEffortTasks[];
	extern const char kNoErrorDialogs[];
	extern const char kProfilingAtStart[];
	extern const char kProfilingFile[];
	extern const char kProfilingFlush[];
	extern const char kTestChildProcess[];
	extern const char kTestDoNotInitializeIcu[];
	extern const char kTraceToFile[];
	extern const char kTraceToFileName[];
	extern const char kV[];
	extern const char kVModule[];
	extern const char kWaitForDebugger[];

	extern const char kDisableHighResTimer[];
	extern const char kDisableUsbKeyboardDetect[];

}  // namespace switches

