#pragma once

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build_config.h"

#if !defined(__clang__)
//#error "Only clang-cl is supported on Windows, see https://crbug.com/988071"
#endif

// Macros for suppressing and disabling warnings on MSVC.
//
// Warning numbers are enumerated at:
// http://msdn.microsoft.com/en-us/library/8x5x43k7(VS.80).aspx
//
// The warning pragma:
// http://msdn.microsoft.com/en-us/library/2c8f766e(VS.80).aspx
//
// Using __pragma instead of #pragma inside macros:
// http://msdn.microsoft.com/en-us/library/d9x1s805.aspx

// MSVC_PUSH_DISABLE_WARNING pushes |n| onto a stack of warnings to be disabled.
// The warning remains disabled until popped by MSVC_POP_WARNING.
#define MSVC_PUSH_DISABLE_WARNING(n) __pragma(warning(push)) \
                                     __pragma(warning(disable:(n)))

// Pop effects of innermost MSVC_PUSH_* macro.
#define MSVC_POP_WARNING() __pragma(warning(pop))

// These macros can be helpful when investigating compiler bugs or when
// investigating issues in local optimized builds, by temporarily disabling
// optimizations for a single function or file. These macros should never be
// used to permanently work around compiler bugs or other mysteries, and should
// not be used in landed changes.
#if !defined(OFFICIAL_BUILD)
#define DISABLE_OPTIMIZE() __pragma(optimize("", off))
#define ENABLE_OPTIMIZE() __pragma(optimize("", on))
#endif  // !defined(OFFICIAL_BUILD)

// Annotate a variable indicating it's ok if the variable is not used.
// (Typically used to silence a compiler warning when the assignment
// is important for some other reason.)
// Use like:
//   int x = ...;
//   ALLOW_UNUSED_LOCAL(x);
#define ALLOW_UNUSED_LOCAL(x) (void)x

// Annotate a typedef or function indicating it's ok if it's not used.
// Use like:
//   typedef Foo Bar ALLOW_UNUSED_TYPE;
#define ALLOW_UNUSED_TYPE

// Annotate a function indicating it should not be inlined.
// Use like:
//   NOINLINE void DoStuff() { ... }
#define NOINLINE __declspec(noinline)

#if defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

// Specify memory alignment for structs, classes, etc.
// Use like:
//   class ALIGNAS(16) MyClass { ... }
//   ALIGNAS(16) int array[4];
//
// In most places you can use the C++11 keyword "alignas", which is preferred.
//
// But compilers have trouble mixing __attribute__((...)) syntax with
// alignas(...) syntax.
//
// Doesn't work in clang or gcc:
//   struct alignas(16) __attribute__((packed)) S { char c; };
// Works in clang but not gcc:
//   struct __attribute__((packed)) alignas(16) S2 { char c; };
// Works in clang and gcc:
//   struct alignas(16) S3 { char c; } __attribute__((packed));
//
// There are also some attributes that must be specified *before* a class
// definition: visibility (used for exporting functions/classes) is one of
// these attributes. This means that it is not possible to use alignas() with a
// class that is marked as exported.
#define ALIGNAS(byte_alignment) __declspec(align(byte_alignment))

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
// To explicitly ignore a result, see |ignore_result()| in base/macros.h.
#undef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT

// Tell the compiler a function is using a printf-style format string.
// |format_param| is the one-based index of the format string parameter;
// |dots_param| is the one-based index of the "..." parameter.
// For v*printf functions (which take a va_list), pass 0 for dots_param.
// (This is undocumented but matches what the system C headers do.)
// For member functions, the implicit this parameter counts as index 1.
#define PRINTF_FORMAT(format_param, dots_param)

// WPRINTF_FORMAT is the same, but for wide format strings.
// This doesn't appear to yet be implemented in any compiler.
// See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38308 .
#define WPRINTF_FORMAT(format_param, dots_param)
// If available, it would look like:
//   __attribute__((format(wprintf, format_param, dots_param)))

// Sanitizers annotations.
#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define NO_SANITIZE(what) __attribute__((no_sanitize(what)))
#endif
#endif
#if !defined(NO_SANITIZE)
#define NO_SANITIZE(what)
#endif

// MemorySanitizer annotations.
#if defined(MEMORY_SANITIZER) && !defined(OS_NACL)
#include <sanitizer/msan_interface.h>

// Mark a memory region fully initialized.
// Use this to annotate code that deliberately reads uninitialized data, for
// example a GC scavenging root set pointers from the stack.
#define MSAN_UNPOISON(p, size)  __msan_unpoison(p, size)

// Check a memory region for initializedness, as if it was being used here.
// If any bits are uninitialized, crash with an MSan report.
// Use this to sanitize data which MSan won't be able to track, e.g. before
// passing data to another process via shared memory.
#define MSAN_CHECK_MEM_IS_INITIALIZED(p, size) \
    __msan_check_mem_is_initialized(p, size)
#else  // MEMORY_SANITIZER
#define MSAN_UNPOISON(p, size)
#define MSAN_CHECK_MEM_IS_INITIALIZED(p, size)
#endif  // MEMORY_SANITIZER

// DISABLE_CFI_PERF -- Disable Control Flow Integrity for perf reasons.
#if !defined(DISABLE_CFI_PERF)
#define DISABLE_CFI_PERF
#endif

// Macro useful for writing cross-platform function pointers.
#if !defined(CDECL)
#define CDECL __cdecl
#endif  // !defined(CDECL)

// Macro for hinting that an expression is likely to be false.
#if !defined(UNLIKELY)
#define UNLIKELY(x) (x)
#endif  // !defined(UNLIKELY)

#if !defined(LIKELY)
#define LIKELY(x) (x)
#endif  // !defined(LIKELY)

// Compiler feature-detection.
// clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
#if defined(__has_feature)
#define HAS_FEATURE(FEATURE) __has_feature(FEATURE)
#else
#define HAS_FEATURE(FEATURE) 0
#endif

// Macro for telling -Wimplicit-fallthrough that a fallthrough is intentional.
#if defined(__clang__)
#define FALLTHROUGH [[clang::fallthrough]]
#else
#define FALLTHROUGH
#endif 

#define PRETTY_FUNCTION __FUNCSIG__
