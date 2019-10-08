// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base_export.h"
#include "callback.h"
#include "logging.h"
#include "macros.h"
#include <optional>
#include "sequence_checker.h"
#include "synchronization/atomic_flag.h"
#include "task/single_thread_task_runner_thread_mode.h"
#include "task/task_executor.h"
#include "task/task_traits.h"
#include "task/thread_pool/delayed_task_manager.h"
#include "task/thread_pool/environment_config.h"
#include "task/thread_pool/pooled_single_thread_task_runner_manager.h"
#include "task/thread_pool/pooled_task_runner_delegate.h"
#include "task/thread_pool/task_source.h"
#include "task/thread_pool/task_tracker.h"
#include "task/thread_pool/thread_group.h"
#include "task/thread_pool/thread_group_impl.h"
#include "task/thread_pool/thread_pool_instance.h"
#include "updateable_sequenced_task_runner.h"
#include "build_config.h"

#include "win/com_init_check_hook.h"

namespace base {

	class Thread;

	namespace internal {

		// Default ThreadPoolInstance implementation. This class is thread-safe.
		class BASE_EXPORT ThreadPoolImpl : public ThreadPoolInstance,
			public TaskExecutor,
			public ThreadGroup::Delegate,
			public PooledTaskRunnerDelegate {
		public:
			using TaskTrackerImpl =
				TaskTracker;

			// Creates a ThreadPoolImpl with a production TaskTracker. |histogram_label|
			// is used to label histograms, No histograms are recorded if it is empty.
			explicit ThreadPoolImpl(std::string_view histogram_label);

			// For testing only. Creates a ThreadPoolImpl with a custom TaskTracker
			ThreadPoolImpl(std::string_view histogram_label,
                 		   std::unique_ptr<TaskTrackerImpl> task_tracker);

			~ThreadPoolImpl() override;

			// ThreadPoolInstance:
			void Start(const ThreadPoolInstance::InitParams& init_params,
				WorkerThreadObserver* worker_thread_observer) override;
			int GetMaxConcurrentNonBlockedTasksWithTraitsDeprecated(
				const TaskTraits& traits) const override;
			void Shutdown() override;
			void FlushForTesting() override;
			void FlushAsyncForTesting(OnceClosure flush_callback) override;
			void JoinForTesting() override;
			void SetHasFence(bool has_fence) override;
			void SetHasBestEffortFence(bool has_best_effort_fence) override;

			// TaskExecutor:
			bool PostDelayedTask(const Location& from_here,
				const TaskTraits& traits,
				OnceClosure task,
				TimeDelta delay) override;
			scoped_refptr<TaskRunner> CreateTaskRunner(const TaskTraits& traits) override;
			scoped_refptr<SequencedTaskRunner> CreateSequencedTaskRunner(
				const TaskTraits& traits) override;
			scoped_refptr<SingleThreadTaskRunner> CreateSingleThreadTaskRunner(
				const TaskTraits& traits,
				SingleThreadTaskRunnerThreadMode thread_mode) override;
			scoped_refptr<SingleThreadTaskRunner> CreateCOMSTATaskRunner(
				const TaskTraits& traits,
				SingleThreadTaskRunnerThreadMode thread_mode) override;
			scoped_refptr<UpdateableSequencedTaskRunner>
				CreateUpdateableSequencedTaskRunner(const TaskTraits& traits);

			// PooledTaskRunnerDelegate:
			bool EnqueueJobTaskSource(scoped_refptr<JobTaskSource> task_source) override;
			void RemoveJobTaskSource(scoped_refptr<JobTaskSource> task_source) override;
			void UpdatePriority(scoped_refptr<TaskSource> task_source,
								TaskPriority priority) override;

			// Returns the TimeTicks of the next task scheduled on ThreadPool (Now() if
			// immediate, nullopt if none). This is thread-safe, i.e., it's safe if tasks
			// are being posted in parallel with this call but such a situation obviously
			// results in a race as to whether this call will see the new tasks in time.
			std::optional<TimeTicks> NextScheduledRunTimeForTesting() const;

			// Forces ripe delayed tasks to be posted (e.g. when time is mocked and
			// advances faster than the real-time delay on ServiceThread).
			void ProcessRipeDelayedTasksForTesting();

		private:
			// Invoked after |has_fence_| or |has_best_effort_fence_| is updated. Sets the
			// CanRunPolicy in TaskTracker and wakes up workers as appropriate.
			void UpdateCanRunPolicy();

			// Returns |traits|, with priority set to TaskPriority::USER_BLOCKING if
			// |all_tasks_user_blocking_| is set.
			TaskTraits SetUserBlockingPriorityIfNeeded(TaskTraits traits) const;

			void ReportHeartbeatMetrics() const;

			const ThreadGroup* GetThreadGroupForTraits(const TaskTraits& traits) const;

			// ThreadGroup::Delegate:
			ThreadGroup* GetThreadGroupForTraits(const TaskTraits& traits) override;

			// Posts |task| to be executed by the appropriate thread group as part of
			// |sequence|. This must only be called after |task| has gone through
			// TaskTracker::WillPostTask() and after |task|'s delayed run time.
			bool PostTaskWithSequenceNow(Task task, scoped_refptr<Sequence> sequence);

			// PooledTaskRunnerDelegate:
			bool PostTaskWithSequence(Task task,
				scoped_refptr<Sequence> sequence) override;
			bool IsRunningPoolWithTraits(const TaskTraits& traits) const override;
			bool ShouldYield(const TaskSource* task_source) const override;

			const std::unique_ptr<TaskTrackerImpl> task_tracker_{};
			std::unique_ptr<Thread> service_thread_{};
			DelayedTaskManager delayed_task_manager_;
			PooledSingleThreadTaskRunnerManager single_thread_task_runner_manager_;

			// Indicates that all tasks are handled as if they had been posted with
			// TaskPriority::USER_BLOCKING. Since this is set in Start(), it doesn't apply
			// to tasks posted before Start() or to tasks posted to TaskRunners created
			// before Start().
			//
			// TODO(fdoray): Remove after experiment. https://crbug.com/757022
			AtomicFlag all_tasks_user_blocking_;

			std::unique_ptr<ThreadGroup> foreground_thread_group_{};
			std::unique_ptr<ThreadGroupImpl> background_thread_group_{};

			// Whether this TaskScheduler was started. Access controlled by
			// |sequence_checker_|.
			bool started_ = false;

			// Whether the --disable-best-effort-tasks switch is preventing execution of
			// BEST_EFFORT tasks until shutdown.
			const bool has_disable_best_effort_switch_{};

			// Whether a fence is preventing execution of tasks of any/BEST_EFFORT
			// priority. Access controlled by |sequence_checker_|.
			bool has_fence_ = false;
			bool has_best_effort_fence_ = false;

#if DCHECK_IS_ON()
			// Set once JoinForTesting() has returned.
			AtomicFlag join_for_testing_returned_;
#endif

#if defined(COM_INIT_CHECK_HOOK_ENABLED)
			// Provides COM initialization verification for supported builds.
			win::ComInitCheckHook com_init_check_hook_;
#endif

			// Asserts that operations occur in sequence with Start().
			SEQUENCE_CHECKER(sequence_checker_);

			TrackedRefFactory<ThreadGroup::Delegate> tracked_ref_factory_;

			DISALLOW_COPY_AND_ASSIGN(ThreadPoolImpl);
		};

	}  // namespace internal
}  // namespace base
