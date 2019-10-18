// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include "threading/thread.h"

#include "bind.h"
#include "bind_helpers.h"
#include "lazy_instance.h"
#include "location.h"
#include "logging.h"
#include "memory/ptr_util.h"
#include "memory/scoped_refptr.h"
#include "message_loop/message_loop_current.h"
#include "message_loop/message_pump.h"
#include "run_loop.h"
#include "synchronization/waitable_event.h"
#include "task/sequence_manager/sequence_manager_impl.h"
#include "task/sequence_manager/task_queue.h"
#include "task/simple_task_executor.h"
#include "third_party/dynamic_annotations/dynamic_annotations.h"
#include "threading/thread_id_name_manager.h"
#include "threading/thread_local.h"
#include "threading/thread_restrictions.h"
#include "threading/thread_task_runner_handle.h"
#include "build_config.h"

#include "win/scoped_com_initializer.h"

namespace base {

	namespace {

		// We use this thread-local variable to record whether or not a thread exited
		// because its Stop method was called.  This allows us to catch cases where
		// MessageLoop::QuitWhenIdle() is called directly, which is unexpected when
		// using a Thread to setup and run a MessageLoop.
		LazyInstance<ThreadLocalBoolean>::Leaky lazy_tls_bool =
			LAZY_INSTANCE_INITIALIZER;

		class SequenceManagerThreadDelegate : public Thread::Delegate {
		public:
			explicit SequenceManagerThreadDelegate(
				MessagePumpType message_pump_type,
				OnceCallback<std::unique_ptr<MessagePump>()> message_pump_factory)
				: sequence_manager_(
					sequence_manager::internal::SequenceManagerImpl::CreateUnbound(
						sequence_manager::SequenceManager::Settings::Builder()
						.SetMessagePumpType(message_pump_type)
						.Build())),
				default_task_queue_(sequence_manager_->CreateTaskQueue(
					sequence_manager::TaskQueue::Spec("default_tq"))),
				message_pump_factory_(std::move(message_pump_factory)) {
				sequence_manager_->SetDefaultTaskRunner(default_task_queue_->task_runner());
			}

			~SequenceManagerThreadDelegate() override = default;

			scoped_refptr<SingleThreadTaskRunner> GetDefaultTaskRunner() override {
				// Surprisingly this might not be default_task_queue_->task_runner() which
				// we set in the constructor. The Thread::Init() method could create a
				// SequenceManager on top of the current one and call
				// SequenceManager::SetDefaultTaskRunner which would propagate the new
				// TaskRunner down to our SequenceManager. Turns out, code actually relies
				// on this and somehow relies on
				// SequenceManagerThreadDelegate::GetDefaultTaskRunner returning this new
				// TaskRunner. So instead of returning default_task_queue_->task_runner() we
				// need to query the SequenceManager for it.
				// The underlying problem here is that Subclasses of Thread can do crazy
				// stuff in Init() but they are not really in control of what happens in the
				// Thread::Delegate, as this is passed in on calling StartWithOptions which
				// could happen far away from where the Thread is created. We should
				// consider getting rid of StartWithOptions, and pass them as a constructor
				// argument instead.
				return sequence_manager_->GetTaskRunner();
			}

			void BindToCurrentThread(TimerSlack timer_slack) override {
				sequence_manager_->BindToMessagePump(
					std::move(message_pump_factory_).Run());
				sequence_manager_->SetTimerSlack(timer_slack);
				simple_task_executor_.emplace(GetDefaultTaskRunner());
			}

		private:
			std::unique_ptr<sequence_manager::internal::SequenceManagerImpl>
				sequence_manager_;
			scoped_refptr<sequence_manager::TaskQueue> default_task_queue_;
			OnceCallback<std::unique_ptr<MessagePump>()> message_pump_factory_;
			std::optional<SimpleTaskExecutor> simple_task_executor_;
		};

	}  // namespace

	Thread::Options::Options() = default;

	Thread::Options::Options(MessagePumpType type, size_t size)
	    : message_pump_type(type), stack_size(size) {}

	Thread::Options::Options(Options&& other) noexcept = default;

	Thread::Options::~Options() = default;

	Thread::Thread(const std::string& name)
		: id_event_(WaitableEvent::ResetPolicy::MANUAL,
			WaitableEvent::InitialState::NOT_SIGNALED),
		name_(name),
		start_event_(WaitableEvent::ResetPolicy::MANUAL,
			WaitableEvent::InitialState::NOT_SIGNALED) {
		// Only bind the sequence on Start(): the state is constant between
		// construction and Start() and it's thus valid for Start() to be called on
		// another sequence as long as every other operation is then performed on that
		// sequence.
		owning_sequence_checker_.DetachFromSequence();
	}

	Thread::~Thread() {
		Stop();
	}

	bool Thread::Start() {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());

		Options options;
		if (com_status_ == STA)
			options.message_pump_type = MessagePumpType::UI;
		return StartWithOptions(options);
	}

	bool Thread::StartWithOptions(const Options& options) {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		DCHECK(!delegate_);
		DCHECK(!IsRunning());
		DCHECK(!stopping_) << "Starting a non-joinable thread a second time? That's "
			<< "not allowed!";
		DCHECK((com_status_ != STA) ||
			   (options.message_pump_type == MessagePumpType::UI));

		// Reset |id_| here to support restarting the thread.
		id_event_.Reset();
		id_ = kInvalidThreadId;

		SetThreadWasQuitProperly(false);

		timer_slack_ = options.timer_slack;

		if (options.delegate) {
			DCHECK(!options.message_pump_factory);
			delegate_ = WrapUnique(options.delegate);
		} else if (options.message_pump_factory) {
		    delegate_ = std::make_unique<SequenceManagerThreadDelegate>(
		        MessagePumpType::CUSTOM, options.message_pump_factory);
		} else {
		    delegate_ = std::make_unique<SequenceManagerThreadDelegate>(
		        options.message_pump_type,
		        BindOnce([](MessagePumpType type) { return MessagePump::Create(type); },
		                 options.message_pump_type));
		}

		start_event_.Reset();

		// Hold |thread_lock_| while starting the new thread to synchronize with
		// Stop() while it's not guaranteed to be sequenced (until crbug/629139 is
		// fixed).
		{
			AutoLock lock(thread_lock_);
			bool success =
				options.joinable
				? PlatformThread::CreateWithPriority(options.stack_size, this,
					&thread_, options.priority)
				: PlatformThread::CreateNonJoinableWithPriority(
					options.stack_size, this, options.priority);
			if (!success) {
				DLOG(ERROR) << "failed to create thread";
				return false;
			}
		}

		joinable_ = options.joinable;

		return true;
	}

	bool Thread::StartAndWaitForTesting() {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		bool result = Start();
		if (!result)
			return false;
		WaitUntilThreadStarted();
		return true;
	}

	bool Thread::WaitUntilThreadStarted() const {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		if (!delegate_)
			return false;
		// https://crbug.com/918039
		ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
		start_event_.Wait();
		return true;
	}

	void Thread::FlushForTesting() const {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		if (!delegate_)
			return;

		WaitableEvent done(WaitableEvent::ResetPolicy::AUTOMATIC,
			WaitableEvent::InitialState::NOT_SIGNALED);
		task_runner()->PostTask(FROM_HERE,
			BindOnce(&WaitableEvent::Signal, Unretained(&done)));
		done.Wait();
	}

	void Thread::Stop() {
		DCHECK(joinable_);

		// TODO(gab): Fix improper usage of this API (http://crbug.com/629139) and
		// enable this check, until then synchronization with Start() via
		// |thread_lock_| is required...
		// DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		AutoLock lock(thread_lock_);

		StopSoon();

		// Can't join if the |thread_| is either already gone or is non-joinable.
		if (thread_.is_null())
			return;

		// Wait for the thread to exit.
		//
		// TODO(darin): Unfortunately, we need to keep |delegate_| around
		// until the thread exits. Some consumers are abusing the API. Make them stop.
		PlatformThread::Join(thread_);
		thread_ = PlatformThreadHandle();

		// The thread should release |delegate_| on exit (note: Join() adds
		// an implicit memory barrier and no lock is thus required for this check).
		DCHECK(!delegate_);

		stopping_ = false;
	}

	void Thread::StopSoon() {
		// TODO(gab): Fix improper usage of this API (http://crbug.com/629139) and
		// enable this check.
		// DCHECK(owning_sequence_checker_.CalledOnValidSequence());

		if (stopping_ || !delegate_)
			return;

		stopping_ = true;

		task_runner()->PostTask(
			FROM_HERE, BindOnce(&Thread::ThreadQuitHelper, Unretained(this)));
	}

	void Thread::DetachFromSequence() {
		DCHECK(owning_sequence_checker_.CalledOnValidSequence());
		owning_sequence_checker_.DetachFromSequence();
	}

	PlatformThreadId Thread::GetThreadId() const {
		// If the thread is created but not started yet, wait for |id_| being ready.
		ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
		id_event_.Wait();
		return id_;
	}

	bool Thread::IsRunning() const {
		// TODO(gab): Fix improper usage of this API (http://crbug.com/629139) and
		// enable this check.
		// DCHECK(owning_sequence_checker_.CalledOnValidSequence());

		// If the thread's already started (i.e. |delegate_| is non-null) and
		// not yet requested to stop (i.e. |stopping_| is false) we can just return
		// true. (Note that |stopping_| is touched only on the same sequence that
		// starts / started the new thread so we need no locking here.)
		if (delegate_ && !stopping_)
			return true;
		// Otherwise check the |running_| flag, which is set to true by the new thread
		// only while it is inside Run().
		AutoLock lock(running_lock_);
		return running_;
	}

	void Thread::Run(RunLoop* run_loop) {
		// Overridable protected method to be called from our |thread_| only.
		DCHECK(id_event_.IsSignaled());
		DCHECK_EQ(id_, PlatformThread::CurrentId());

		run_loop->Run();
	}

	// static
	void Thread::SetThreadWasQuitProperly(bool flag) {
		lazy_tls_bool.Pointer()->Set(flag);
	}

	// static
	bool Thread::GetThreadWasQuitProperly() {
		bool quit_properly = true;
#if DCHECK_IS_ON()
		quit_properly = lazy_tls_bool.Pointer()->Get();
#endif
		return quit_properly;
	}

	void Thread::ThreadMain() {
		// First, make GetThreadId() available to avoid deadlocks. It could be called
		// any place in the following thread initialization code.
		DCHECK(!id_event_.IsSignaled());
		// Note: this read of |id_| while |id_event_| isn't signaled is exceptionally
		// okay because ThreadMain has a happens-after relationship with the other
		// write in StartWithOptions().
		DCHECK_EQ(kInvalidThreadId, id_);
		id_ = PlatformThread::CurrentId();
		DCHECK_NE(kInvalidThreadId, id_);
		id_event_.Signal();

		// Complete the initialization of our Thread object.
		PlatformThread::SetName(name_);
		ANNOTATE_THREAD_NAME(name_.c_str());  // Tell the name to race detector.

		// Lazily initialize the |message_loop| so that it can run on this thread.
		DCHECK(delegate_);
		// This binds MessageLoopCurrent and ThreadTaskRunnerHandle.
		delegate_->BindToCurrentThread(timer_slack_);
		DCHECK(MessageLoopCurrent::Get());
		DCHECK(ThreadTaskRunnerHandle::IsSet());

		std::unique_ptr<win::ScopedCOMInitializer> com_initializer;
		if (com_status_ != NONE) {
		    com_initializer.reset(
		        (com_status_ == STA)
		            ? new win::ScopedCOMInitializer()
		            : new win::ScopedCOMInitializer(win::ScopedCOMInitializer::kMTA));
		}

		// Let the thread do extra initialization.
		Init();

		{
			AutoLock lock(running_lock_);
			running_ = true;
		}

		start_event_.Signal();

		RunLoop run_loop;
		run_loop_ = &run_loop;
		Run(run_loop_);

		{
			AutoLock lock(running_lock_);
			running_ = false;
		}

		// Let the thread do extra cleanup.
		CleanUp();

		com_initializer.reset();

		DCHECK(GetThreadWasQuitProperly());

		// We can't receive messages anymore.
		// (The message loop is destructed at the end of this block)
		delegate_.reset();
		run_loop_ = nullptr;
	}

	void Thread::ThreadQuitHelper() const {
		DCHECK(run_loop_);
		run_loop_->QuitWhenIdle();
		SetThreadWasQuitProperly(true);
	}

}  // namespace base
