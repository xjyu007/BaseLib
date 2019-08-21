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
// Author: Sanjay Ghemawat
//
// Some of our malloc implementations can invoke the following hooks whenever
// memory is allocated or deallocated.  MallocHook is thread-safe, and things
// you do before calling AddFooHook(MyHook) are visible to any resulting calls
// to MyHook.  Hooks must be thread-safe.  If you write:
//
//   CHECK(MallocHook::AddNewHook(&MyNewHook));
//
// MyNewHook will be invoked in subsequent calls in the current thread, but
// there are no guarantees on when it might be invoked in other threads.
//
// There are a limited number of slots available for each hook type.  Add*Hook
// will return false if there are no slots available.  Remove*Hook will return
// false if the given hook was not already installed.
//
// The order in which individual hooks are called in Invoke*Hook is undefined.
//
// It is safe for a hook to remove itself within Invoke*Hook and add other
// hooks.  Any hooks added inside a hook invocation (for the same hook type)
// will not be invoked for the current invocation.
//
// One important user of these hooks is the heap profiler.
//
// CAVEAT: If you add new MallocHook::Invoke* calls then those calls must be
// directly in the code of the (de)allocation function that is provided to the
// user and that function must have an ATTRIBUTE_SECTION(malloc_hook) attribute.
//
// Note: the Invoke*Hook() functions are defined in malloc_hook-inl.h.  If you
// need to invoke a hook (which you shouldn't unless you're part of tcmalloc),
// be sure to #include malloc_hook-inl.h in addition to malloc_hook.h.
//
// NOTE FOR C USERS: If you want to use malloc_hook functionality from
// a C program, #include malloc_hook_c.h instead of this file.

#ifndef _MALLOC_HOOK_H_
#define _MALLOC_HOOK_H_

// The C++ methods below call the C version (MallocHook_*), and thus
// convert between an int and a bool.  Windows complains about this
// (a "performance warning") which we don't care about, so we suppress.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

typedef void (*MallocHook_NewHook)(const void* ptr, size_t size);
typedef MallocHook_NewHook NewHook;
typedef void (*MallocHook_DeleteHook)(const void* ptr);
typedef MallocHook_DeleteHook DeleteHook;

class MallocHook {
public:

	// Get the current stack trace.  Try to skip all routines up to and
	// and including the caller of MallocHook::Invoke*.
	// Use "skip_count" (similarly to GetStackTrace from stacktrace.h)
	// as a hint about how many routines to skip if better information
	// is not available.
	static int GetCallerStackTrace(void** result, int max_depth, int skip_count);

	static NewHook SetNewHook(NewHook hook);

	static DeleteHook SetDeleteHook(DeleteHook hook); 
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif /* _MALLOC_HOOK_H_ */
