#pragma once

#if defined(_WIN32)
#define OS_WIN 1
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#else
#error Please add support for your platform in build_config.h
#endif

// Compiler detection.
#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#error Please add support for your compiler in build_config.h
#endif

// Processor architecture detection.  For more info on what's defined, see:
//   http://msdn.microsoft.com/en-us/library/b0084kay.aspx
//   http://www.agner.org/optimize/calling_conventions.pdf
//   or with gcc, run: "echo | gcc -E -dM -"
#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#else
#error Please add support for your architecture in build_config.h
#endif

// Type detection for wchar_t.
#if defined(OS_WIN)
#define WCHAR_T_IS_UTF16
#else
#error Please add support for your compiler in build_config.h
#endif