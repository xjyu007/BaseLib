// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <utility>

#include "atomic_ref_count.h"
#include "base_export.h"
#include "compiler_specific.h"
#include "logging.h"
#include "macros.h"
#include "memory/scoped_refptr.h"
#include "sequence_checker.h"
#include "threading/thread_collision_warner.h"
#include "build_config.h"

namespace base {
	namespace subtle {

		class BASE_EXPORT RefCountedBase {
		public:
			bool HasOneRef() const { return ref_count_ == 1; }
			bool HasAtLeastOneRef() const { return ref_count_ >= 1; }

		protected:
			explicit RefCountedBase(StartRefCountFromZeroTag) {
#if DCHECK_IS_ON()
				sequence_checker_.DetachFromSequence();
#endif
			}

			explicit RefCountedBase(StartRefCountFromOneTag) : ref_count_(1) {
#if DCHECK_IS_ON()
				needs_adopt_ref_ = true;
				sequence_checker_.DetachFromSequence();
#endif
			}

			~RefCountedBase() {
#if DCHECK_IS_ON()
				DCHECK(in_dtor_) << "RefCounted object deleted without calling Release()";
#endif
			}

			void AddRef() const {
				// TODO(maruel): Add back once it doesn't assert 500 times/sec.
				// Current thread books the critical section "AddRelease"
				// without release it.
				// DFAKE_SCOPED_LOCK_THREAD_LOCKED(add_release_);
#if DCHECK_IS_ON()
				DCHECK(!in_dtor_);
				DCHECK(!needs_adopt_ref_)
					<< "This RefCounted object is created with non-zero reference count."
					<< " The first reference to such a object has to be made by AdoptRef or"
					<< " MakeRefCounted.";
				if (ref_count_ >= 1) {
					DCHECK(CalledOnValidSequence());
				}
#endif

				AddRefImpl();
			}

			// Returns true if the object should self-delete.
			bool Release() const {
    			ReleaseImpl();

				// TODO(maruel): Add back once it doesn't assert 500 times/sec.
				// Current thread books the critical section "AddRelease"
				// without release it.
				// DFAKE_SCOPED_LOCK_THREAD_LOCKED(add_release_);

#if DCHECK_IS_ON()
				DCHECK(!in_dtor_);
				if (ref_count_ == 0)
					in_dtor_ = true;

				if (ref_count_ >= 1)
					DCHECK(CalledOnValidSequence());
				if (ref_count_ == 1)
					sequence_checker_.DetachFromSequence();
#endif

				return ref_count_ == 0;
			}

		private:
			template <typename U>
			friend scoped_refptr<U> base::AdoptRef(U*);

			//FRIEND_TEST_ALL_PREFIXES(RefCountedDeathTest, TestOverflowCheck);

			void Adopted() const {
#if DCHECK_IS_ON()
				DCHECK(needs_adopt_ref_);
				needs_adopt_ref_ = false;
#endif
			}

#if defined(ARCH_CPU_64_BITS)
			void AddRefImpl() const;
			void ReleaseImpl() const;
#else
			void AddRefImpl() const { ++ref_count_; }
			void ReleaseImpl() const { --ref_count_; }
#endif

#if DCHECK_IS_ON()
			bool CalledOnValidSequence() const;
#endif

			mutable uint32_t ref_count_ = 0;
			static_assert(std::is_unsigned<decltype(ref_count_)>::value,
			              "ref_count_ must be an unsigned type.");

#if DCHECK_IS_ON()
			mutable bool needs_adopt_ref_ = false;
			mutable bool in_dtor_ = false;
			mutable SequenceChecker sequence_checker_;
#endif

			DFAKE_MUTEX(add_release_);

			DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
		};

		class BASE_EXPORT RefCountedThreadSafeBase {
		public:
			bool HasOneRef() const;
			bool HasAtLeastOneRef() const;

		protected:
			explicit constexpr RefCountedThreadSafeBase(StartRefCountFromZeroTag) {}
			explicit constexpr RefCountedThreadSafeBase(StartRefCountFromOneTag)
				: ref_count_(1) {
#if DCHECK_IS_ON()
				needs_adopt_ref_ = true;
#endif
			}

#if DCHECK_IS_ON()
			~RefCountedThreadSafeBase();
#else
			~RefCountedThreadSafeBase() = default;
#endif

			// Release and AddRef are suitable for inlining on X86 because they generate
			// very small code sequences. On other platforms (ARM), it causes a size
			// regression and is probably not worth it.
#if defined(ARCH_CPU_X86_FAMILY)
  // Returns true if the object should self-delete.
			bool Release() const { return ReleaseImpl(); }
			void AddRef() const { AddRefImpl(); }
			void AddRefWithCheck() const { AddRefWithCheckImpl(); }
#else
  // Returns true if the object should self-delete.
			bool Release() const;
			void AddRef() const;
			void AddRefWithCheck() const;
#endif

		private:
			template <typename U>
			friend scoped_refptr<U> base::AdoptRef(U*);

			void Adopted() const {
#if DCHECK_IS_ON()
				DCHECK(needs_adopt_ref_);
				needs_adopt_ref_ = false;
#endif
			}

			ALWAYS_INLINE void AddRefImpl() const {
#if DCHECK_IS_ON()
				DCHECK(!in_dtor_);
				DCHECK(!needs_adopt_ref_)
					<< "This RefCounted object is created with non-zero reference count."
					<< " The first reference to such a object has to be made by AdoptRef or"
					<< " MakeRefCounted.";
#endif
				ref_count_.Increment();
			}

			ALWAYS_INLINE void AddRefWithCheckImpl() const {
#if DCHECK_IS_ON()
				DCHECK(!in_dtor_);
				DCHECK(!needs_adopt_ref_)
					<< "This RefCounted object is created with non-zero reference count."
					<< " The first reference to such a object has to be made by AdoptRef or"
					<< " MakeRefCounted.";
#endif
				CHECK(ref_count_.Increment() > 0);
			}

			ALWAYS_INLINE bool ReleaseImpl() const {
#if DCHECK_IS_ON()
				DCHECK(!in_dtor_);
				DCHECK(!ref_count_.IsZero());
#endif
				if (!ref_count_.Decrement()) {
#if DCHECK_IS_ON()
					in_dtor_ = true;
#endif
					return true;
				}
				return false;
			}

			mutable AtomicRefCount ref_count_{ 0 };
#if DCHECK_IS_ON()
			mutable bool needs_adopt_ref_ = false;
			mutable bool in_dtor_ = false;
#endif

			DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
		};

	}  // namespace subtle

	//
	// A base class for reference counted classes.  Otherwise, known as a cheap
	// knock-off of WebKit's RefCounted<T> class.  To use this, just extend your
	// class from it like so:
	//
	//   class MyFoo : public base::RefCounted<MyFoo> {
	//    ...
	//    private:
	//     friend class base::RefCounted<MyFoo>;
	//     ~MyFoo();
	//   };
	//
	// You should always make your destructor non-public, to avoid any code deleting
	// the object accidently while there are references to it.
	//
	//
	// The ref count manipulation to RefCounted is NOT thread safe and has DCHECKs
	// to trap unsafe cross thread usage. A subclass instance of RefCounted can be
	// passed to another execution sequence only when its ref count is 1. If the ref
	// count is more than 1, the RefCounted class verifies the ref updates are made
	// on the same execution sequence as the previous ones. The subclass can also
	// manually call IsOnValidSequence to trap other non-thread-safe accesses; see
	// the documentation for that method.
	//
	//
	// The reference count starts from zero by default, and we intended to migrate
	// to start-from-one ref count. Put REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() to
	// the ref counted class to opt-in.
	//
	// If an object has start-from-one ref count, the first scoped_refptr need to be
	// created by base::AdoptRef() or base::MakeRefCounted(). We can use
	// base::MakeRefCounted() to create create both type of ref counted object.
	//
	// The motivations to use start-from-one ref count are:
	//  - Start-from-one ref count doesn't need the ref count increment for the
	//    first reference.
	//  - It can detect an invalid object acquisition for a being-deleted object
	//    that has zero ref count. That tends to happen on custom deleter that
	//    delays the deletion.
	//    TODO(tzik): Implement invalid acquisition detection.
	//  - Behavior parity to Blink's WTF::RefCounted, whose count starts from one.
	//    And start-from-one ref count is a step to merge WTF::RefCounted into
	//    base::RefCounted.
	//
#define REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE()             \
    static constexpr ::base::subtle::StartRefCountFromOneTag \
        kRefCountPreference = ::base::subtle::kStartRefCountFromOneTag

	template <class T, typename Traits>
	class RefCounted;

	template <typename T>
	struct DefaultRefCountedTraits {
		static void Destruct(const T* x) {
			RefCounted<T, DefaultRefCountedTraits>::DeleteInternal(x);
		}
	};

	template <class T, typename Traits = DefaultRefCountedTraits<T>>
	class RefCounted : public subtle::RefCountedBase {
	public:
		static constexpr subtle::StartRefCountFromZeroTag kRefCountPreference = 
			subtle::kStartRefCountFromZeroTag;

		RefCounted() : RefCountedBase(T::kRefCountPreference) {}

		void AddRef() const {
			RefCountedBase::AddRef();
		}

		void Release() const {
			if (RefCountedBase::Release()) {
				// Prune the code paths which the static analyzer may take to simulate
				// object destruction. Use-after-free errors aren't possible given the
				// lifetime guarantees of the refcounting system.
				ANALYZER_SKIP_THIS_PATH();

				Traits::Destruct(static_cast<const T*>(this));
			}
		}

	protected:
		~RefCounted() = default;

	private:
		friend struct DefaultRefCountedTraits<T>;
		template <typename U>
		static void DeleteInternal(const U* x) {
			delete x;
		}

		DISALLOW_COPY_AND_ASSIGN(RefCounted);
	};

	// Forward declaration.
	template <class T, typename Traits> class RefCountedThreadSafe;

	// Default traits for RefCountedThreadSafe<T>.  Deletes the object when its ref
	// count reaches 0.  Overload to delete it on a different thread etc.
	template<typename T>
	struct DefaultRefCountedThreadSafeTraits {
		static void Destruct(const T* x) {
			// Delete through RefCountedThreadSafe to make child classes only need to be
			// friend with RefCountedThreadSafe instead of this struct, which is an
			// implementation detail.
			RefCountedThreadSafe<T, 
								 DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
		}
	};

	//
	// A thread-safe variant of RefCounted<T>
	//
	//   class MyFoo : public base::RefCountedThreadSafe<MyFoo> {
	//    ...
	//   };
	//
	// If you're using the default trait, then you should add compile time
	// asserts that no one else is deleting your object.  i.e.
	//    private:
	//     friend class base::RefCountedThreadSafe<MyFoo>;
	//     ~MyFoo();
	//
	// We can use REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() with RefCountedThreadSafe
	// too. See the comment above the RefCounted definition for details.
	template <class T, typename Traits = DefaultRefCountedThreadSafeTraits<T> >
	class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
	public:
		static constexpr subtle::StartRefCountFromZeroTag kRefCountPreference = 
			subtle::kStartRefCountFromZeroTag;

		explicit RefCountedThreadSafe() 
			: RefCountedThreadSafeBase(T::kRefCountPreference) {}

		void AddRef() const { AddRefImpl(T::kRefCountPreference); }

		void Release() const {
			if (RefCountedThreadSafeBase::Release()) {
				ANALYZER_SKIP_THIS_PATH();
				Traits::Destruct(static_cast<const T*>(this));
			}
		}

	protected:
		~RefCountedThreadSafe() = default;

	private:
		friend struct DefaultRefCountedThreadSafeTraits<T>;
		template <typename U>
		static void DeleteInternal(const U* x) {
			delete x;
		}

		void AddRefImpl(subtle::StartRefCountFromZeroTag) const {
			RefCountedThreadSafeBase::AddRef();
		}

		void AddRefImpl(subtle::StartRefCountFromOneTag) const {
			AddRefWithCheck();
		}

		DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
	};

	//
	// A thread-safe wrapper for some piece of data so we can place other
	// things in scoped_refptrs<>.
	//
	template<typename T>
	class RefCountedData 
		: public RefCountedThreadSafe< RefCountedData<T> > {
	public:
		RefCountedData() : data() {}
		RefCountedData(const T& in_value) : data(in_value) {}
		RefCountedData(T&& in_value) : data(std::move(in_value)) {}

		T data;

	private:
		friend class RefCountedThreadSafe<RefCountedData<T> >;
		~RefCountedData() = default;
	};

	template <typename T>
	bool operator==(const RefCountedData<T>& lhs, const RefCountedData<T>& rhs) {
		return lhs.data == rhs.data;
	}

	template <typename T>
	bool operator!=(const RefCountedData<T>& lhs, const RefCountedData<T>& rhs) {
		return !(lhs == rhs);
	}

}  // namespace base

