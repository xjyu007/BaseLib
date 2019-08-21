// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Sanjay Ghemawat <opensource@google.com>
//
// Extra extensions exported by some malloc implementations.  These
// extensions are accessed through a virtual base class so an
// application can link against a malloc that does not implement these
// extensions, and it will get default versions that do nothing.
//
// NOTE FOR C USERS: If you wish to use this functionality from within
// a C program, see malloc_extension_c.h.

#ifndef BASE_MALLOC_EXTENSION_H_
#define BASE_MALLOC_EXTENSION_H_

#include <atomic>
#include <string>
#include <vector>

#ifdef __has_attribute
#define MALLOC_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define MALLOC_HAVE_ATTRIBUTE(x) 0
#endif

#if MALLOC_HAVE_ATTRIBUTE(unused)
#undef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

static const int kMallocHistogramSize = 64;

// One day, we could support other types of writers (perhaps for C?)
typedef std::string MallocExtensionWriter;

namespace base {
	struct MallocRange;
}

// Interface to a pluggable system allocator.
class SysAllocator {
public:
	SysAllocator() {}
	virtual ~SysAllocator();

	// Allocates "size"-byte of memory from system aligned with "alignment".
	// Returns NULL if failed. Otherwise, the returned pointer p up to and
	// including (p + actual_size -1) have been allocated.
	virtual void* Alloc(size_t size, size_t* actual_size, size_t alignment) = 0;
};

// The default implementations of the following routines do nothing.
// All implementations should be thread-safe; the current one
// (TCMallocImplementation) is.
class MallocExtension {
public:
	virtual ~MallocExtension();

	// Call this very early in the program execution -- say, in a global
	// constructor -- to set up parameters and state needed by all
	// instrumented malloc implemenatations.  One example: this routine
	// sets environemnt variables to tell STL to use libc's malloc()
	// instead of doing its own memory management.  This is safe to call
	// multiple times, as long as each time is before threads start up.
	static void Initialize();

	// Outputs to "writer" a sample of live objects and the stack traces
	// that allocated these objects.  The format of the returned output
	// is equivalent to the output of the heap profiler and can
	// therefore be passed to "pprof". This function is equivalent to
	// ReadStackTraces. The main difference is that this function returns
	// serialized data appropriately formatted for use by the pprof tool.
	// NOTE: by default, tcmalloc does not do any heap sampling, and this
	//       function will always return an empty sample.  To get useful
	//       data from GetHeapSample, you must also set the environment
	//       variable TCMALLOC_SAMPLE_PARAMETER to a value such as 524288.
	virtual void GetHeapSample(MallocExtensionWriter * writer);

	// Get the named "property"'s value.  Returns true if the property
	// is known.  Returns false if the property is not a valid property
	// name for the current malloc implementation.
	// REQUIRES: property != NULL; value != NULL
	virtual bool GetNumericProperty(const char* property, size_t * value);

	// Set the named "property"'s value.  Returns true if the property
	// is known and writable.  Returns false if the property is not a
	// valid property name for the current malloc implementation, or
	// is not writable.
	// REQUIRES: property != NULL
	virtual bool SetNumericProperty(const char* property, size_t value);


	// Same as ReleaseToSystem() but release as much memory as possible.
	virtual void ReleaseFreeMemory();

	// The current malloc implementation.  Always non-NULL.
	static MallocExtension* instance() {
		InitModuleOnce();
		return current_instance_.load(std::memory_order_acquire);
	}

private:
	static MallocExtension* InitModule();

	static void InitModuleOnce() {
		// Pointer stored here so heap leak checker will consider the default
		// instance reachable, even if current_instance_ is later overridden by
		// MallocExtension::Register().
		ATTRIBUTE_UNUSED static MallocExtension* default_instance = InitModule();
	}

	static std::atomic<MallocExtension*> current_instance_;
};

namespace base {

	// Information passed per range.  More fields may be added later.
	struct MallocRange {
		enum Type {
			INUSE,                // Application is using this range
			FREE,                 // Range is currently free
			UNMAPPED,             // Backing physical memory has been returned to the OS
			UNKNOWN
			// More enum values may be added in the future
		};

		uintptr_t address;    // Address of range
		size_t length;        // Byte length of range
		Type type;            // Type of this range
		double fraction;      // Fraction of range that is being used (0 if !INUSE)

		// Perhaps add the following:
		// - stack trace if this range was sampled
		// - heap growth stack trace if applicable to this range
		// - age when allocated (for inuse) or freed (if not in use)
	};

} // namespace base

#endif  // BASE_MALLOC_EXTENSION_H_
