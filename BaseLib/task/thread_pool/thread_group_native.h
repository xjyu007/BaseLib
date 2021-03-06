// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "task/thread_pool/thread_group.h"

namespace base {
	namespace internal {

		class BASE_EXPORT ThreadGroupNative : public ThreadGroup {
		public:
			// Destroying a ThreadGroupNative is not allowed in
			// production; it is always leaked. In tests, it can only be destroyed after
			// JoinForTesting() has returned.
			~ThreadGroupNative() override;

			// Starts the thread group and allows tasks to begin running.
			void Start(WorkerEnvironment worker_environment = WorkerEnvironment::NONE);

			// ThreadGroup:
			void JoinForTesting() override;
			size_t GetMaxConcurrentNonBlockedTasksDeprecated() const override;
			void ReportHeartbeatMetrics() const override;
			void DidUpdateCanRunPolicy() override;

		protected:
			ThreadGroupNative(TrackedRef<TaskTracker> task_tracker,
				TrackedRef<Delegate> delegate,
				ThreadGroup* predecessor_thread_group);

			// Runs a task off the next task source on the |priority_queue_|. Called by
			// callbacks posted to platform native thread pools.
			void RunNextTaskSourceImpl();

			virtual void JoinImpl() = 0;
			virtual void StartImpl() = 0;
			virtual void SubmitWork() = 0;

			// Used to control the worker environment. Supports COM MTA on Windows.
			WorkerEnvironment worker_environment_ = WorkerEnvironment::NONE;

		private:
			class ScopedWorkersExecutor;

			// ThreadGroup:
			void UpdateSortKey(TaskSource::Transaction transaction) override;
			void PushTaskSourceAndWakeUpWorkers(
				TransactionWithRegisteredTaskSource transaction_with_task_source)
				override;
			void EnsureEnoughWorkersLockRequired(BaseScopedWorkersExecutor* executor)
				override;

			// Updates the minimum priority allowed to run below which tasks should yield,
			// based on task sources in |priority_queue_|.
			void UpdateMinAllowedPriorityLockRequired();

			// Returns the top TaskSource off the |priority_queue_|. Returns nullptr
			// if the |priority_queue_| is empty.
			RegisteredTaskSource GetWork();

			// Indicates whether the thread group has been started yet.
			bool started_ = false;

			// Number of threadpool work submitted to the thread group which haven't
			// popped a TaskSource from the PriorityQueue yet.
			size_t num_pending_threadpool_work_ = 0;

#if DCHECK_IS_ON()
			// Set once JoinForTesting() has returned.
			bool join_for_testing_returned_ = false;
#endif

			DISALLOW_COPY_AND_ASSIGN(ThreadGroupNative);
		};

	}  // namespace internal
}  // namespace base
