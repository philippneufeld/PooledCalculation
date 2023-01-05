// Philipp Neufeld, 2023

#ifndef CP_PLATFORM_H_
#define CP_PLATFORM_H_

// Includes
#include <cstdint>

// compiler
#if defined(_MSC_VER) && !defined(__clang__)
#	ifndef CP_COMPILER_MSVC
#	define CP_COMPILER_MSVC
#	endif // !CP_COMPILER_MSVC
#elif defined(__clang__)
#	ifndef CP_COMPILER_CLANG
#	define CP_COMPILER_CLANG
#	endif // !CP_COMPILER_CLANG
#elif defined(__GNUC__)
#	ifndef CP_COMPILER_GNUC
#	define CP_COMPILER_GNUC
#	endif // !CP_COMPILER_GNUC
#else
#	ifndef CP_COMPILER_UNKNOWN
#	define CP_COMPILER_UNKNOWN
#	endif // !CP_COMPILER_UNKNOWN
#endif

// OS macro
#if defined (_WIN32)
#	ifndef CP_PLATFORM_WINDOWS
#	define CP_PLATFORM_WINDOWS
#	endif // !CP_PLATFORM_WINDOWS
#elif defined (__linux__)
#	ifndef CP_PLATFORM_LINUX
#	define CP_PLATFORM_LINUX
#	endif // !CP_PLATFORM_LINUX
#elif defined (__APPLE__)
#	ifndef CP_PLATFORM_MACOS
#	define CP_PLATFORM_MACOS
#	endif // !CP_PLATFORM_MACOS
#else
#	ifndef CP_PLATFORM_UNKNOWN
#	define CP_PLATFORM_UNKNOWN
#	endif // !CP_PLATFORM_UNKNOWN
#endif

// architecture
// see https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
#if defined(__x86_64__) || defined(_M_X64)
# define CP_ARCH_X86_64
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
# define CP_ARCH_X86_32
#elif defined(__aarch64__) || defined(_M_ARM64)
# define CP_ARCH_ARM64
#else
# define CP_ARCH_UNKNOWN
#endif

// c++ version
#if defined CP_COMPILER_MSVC
# define CP_CPLUSPLUS _MSVC_LANG
#else
#	define CP_CPLUSPLUS __cplusplus
#endif

#if CP_CPLUSPLUS >= 201103L
#	define CP_HAS_CXX11
#endif

#if CP_CPLUSPLUS >= 201402L
#	define CP_HAS_CXX14
#endif 

#if CP_CPLUSPLUS >= 201703L
#	define CP_HAS_CXX17
#endif 

#if CP_CPLUSPLUS >= 202002L
#	define CP_HAS_CXX20
#endif 

// debug macros
#if defined (DEBUG) || defined (_DEBUG)
#	ifndef CP_DEBUG
#	define CP_DEBUG
#	endif
#endif

// Inline
#if defined(CP_COMPILER_MSVC)
#define CP_ALWAYS_INLINE __forceinline
#elif defined(CP_COMPILER_GNUC) || defined(CP_COMPILER_CLANG)
#define CP_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

// miscellaneous
#ifndef NOEXCEPT
#	define NOEXCEPT noexcept
#endif

namespace CP
{
  // define namespace
}

#endif