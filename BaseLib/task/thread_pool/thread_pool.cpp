// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "task/thread_pool/thread_pool.h"

#include <algorithm>

#include "logging.h"
#include "memory/ptr_util.h"
#include "system/sys_info.h"
#include "task/thread_pool/thread_pool_impl.h"

namespace base {

	namespace {

		// |g_thread_pool| is intentionally leaked on shutdown.
		ThreadPoolInstance* g_thread_pool = nullptr;

	}  // namespace

	ThreadPoolInstance::InitParams::InitParams(int max_num_foreground_threads_in)
		: max_num_foreground_threads(max_num_foreground_threads_in) {}

	ThreadPoolInstance::InitParams::~InitParams() = default;

	ThreadPoolInstance::ScopedExecutionFence::ScopedExecutionFence() {
		DCHECK(g_thread_pool);
		g_thread_pool->SetHasFence(true);
	}

	ThreadPoolInstance::ScopedExecutionFence::~ScopedExecutionFence() {
		DCHECK(g_thread_pool);
		g_thread_pool->SetHasFence(false);
	}

	ThreadPoolInstance::ScopedBestEffortExecutionFence::
		ScopedBestEffortExecutionFence() {
		DCHECK(g_thread_pool);
		g_thread_pool->SetHasBestEffortFence(true);
	}

	ThreadPoolInstance::ScopedBestEffortExecutionFence::
		~ScopedBestEffortExecutionFence() {
		DCHECK(g_thread_pool);
		g_thread_pool->SetHasBestEffortFence(false);
	}

#if !defined(OS_NACL)
	// static
	void ThreadPoolInstance::CreateAndStartWithDefaultParams(std::string_view name) {
		Create(name);
		g_thread_pool->StartWithDefaultParams();
	}

	void ThreadPoolInstance::StartWithDefaultParams() {
		// Values were chosen so that:
		// * There are few background threads.
		// * Background threads never outnumber foreground threads.
		// * The system is utilized maximally by foreground threads.
		// * The main thread is assumed to be busy, cap foreground workers at
		//   |num_cores - 1|.
#undef max
		const auto num_cores = SysInfo::NumberOfProcessors();
		const auto max_num_foreground_threads = std::max(3, num_cores - 1);
		Start({ max_num_foreground_threads });
	}
#endif  // !defined(OS_NACL)

	void ThreadPoolInstance::Create(std::string_view name) {
		Set(std::make_unique<internal::ThreadPoolImpl>(name));
	}

	// static
	void ThreadPoolInstance::Set(std::unique_ptr<ThreadPoolInstance> thread_pool) {
		delete g_thread_pool;
		g_thread_pool = thread_pool.release();
	}

	// static
	ThreadPoolInstance* ThreadPoolInstance::Get() {
		return g_thread_pool;
	}

}  // namespace base
