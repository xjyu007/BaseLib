// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base_export.h"
#include "logging.h"
#include "macros.h"

#include "win/windows_types.h"

namespace base {
	namespace internal {

		// This class implements the underlying platform-specific spin-lock mechanism
		// used for the Lock class.  Most users should not use LockImpl directly, but
		// should instead use Lock.
		class BASE_EXPORT LockImpl {
		public:
			using NativeHandle = CHROME_SRWLOCK;

			LockImpl();
			~LockImpl();

			// If the lock is not held, take it and return true.  If the lock is already
			// held by something else, immediately return false.
			bool Try();

			// Take the lock, blocking until it is available if necessary.
			void Lock();

			// Release the lock.  This must only be called by the lock's holder: after
			// a successful call to Try, or a call to Lock.
			inline void Unlock();

			// Return the native underlying lock.
			// TODO(awalker): refactor lock and condition variables so that this is
			// unnecessary.
			NativeHandle* native_handle() { return &native_handle_; }

		private:
			NativeHandle native_handle_;

			DISALLOW_COPY_AND_ASSIGN(LockImpl);
		};

		void LockImpl::Unlock() {
			::ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&native_handle_));
		}

		// This is an implementation used for AutoLock templated on the lock type.
		template <class LockType>
		class BasicAutoLock {
		public:
			struct AlreadyAcquired {};

			explicit BasicAutoLock(LockType& lock) : lock_(lock) {
				lock_.Acquire();
			}

			BasicAutoLock(LockType& lock, const AlreadyAcquired&) : lock_(lock) {
				lock_.AssertAcquired();
			}

			~BasicAutoLock() {
				lock_.AssertAcquired();
				lock_.Release();
			}

		private:
			LockType& lock_;
			DISALLOW_COPY_AND_ASSIGN(BasicAutoLock);
		};

		// This is an implementation used for AutoUnlock templated on the lock type.
		template <class LockType>
		class BasicAutoUnlock {
		public:
			explicit BasicAutoUnlock(LockType& lock) : lock_(lock) {
				// We require our caller to have the lock.
				lock_.AssertAcquired();
				lock_.Release();
			}

			~BasicAutoUnlock() { lock_.Acquire(); }

		private:
			LockType& lock_;
			DISALLOW_COPY_AND_ASSIGN(BasicAutoUnlock);
		};

		// This is an implementation used for AutoLockMaybe templated on the lock type.
		template <class LockType>
		class BasicAutoLockMaybe {
		public:
			explicit BasicAutoLockMaybe(LockType* lock) : lock_(lock) {
				if (lock_)
					lock_->Acquire();
			}

			~BasicAutoLockMaybe() {
				if (lock_) {
					lock_->AssertAcquired();
					lock_->Release();
				}
			}

		private:
			LockType* const lock_;
			DISALLOW_COPY_AND_ASSIGN(BasicAutoLockMaybe);
		};

		// This is an implementation used for ReleasableAutoLock templated on the lock
		// type.
		template <class LockType>
		class BasicReleasableAutoLock {
		public:
			explicit BasicReleasableAutoLock(LockType* lock) : lock_(lock) {
				DCHECK(lock_);
				lock_->Acquire();
			}

			~BasicReleasableAutoLock() {
				if (lock_) {
					lock_->AssertAcquired();
					lock_->Release();
				}
			}

			void Release() {
				DCHECK(lock_);
				lock_->AssertAcquired();
				lock_->Release();
				lock_ = nullptr;
			}

		private:
			LockType* lock_;
			DISALLOW_COPY_AND_ASSIGN(BasicReleasableAutoLock);
		};

	}  // namespace internal
} // namespace base