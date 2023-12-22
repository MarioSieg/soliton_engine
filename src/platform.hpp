// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#define LU_STRINGIZE_IMPL(x) #x
#define LU_STRINGIZE(x) LU_STRINGIZE_IMPL(x)

// Architecture
#define LU_ARCH_32BIT 0
#define LU_ARCH_64BIT 0

// Compiler
#define LU_COMPILER_CLANG          0
#define LU_COMPILER_CLANG_ANALYZER 0
#define LU_COMPILER_GCC            0
#define LU_COMPILER_MSVC           0

// Endianness
#define LU_CPU_ENDIAN_BIG    0
#define LU_CPU_ENDIAN_LITTLE 0

// CPU
#define LU_CPU_ARM   0
#define LU_CPU_JIT   0
#define LU_CPU_MIPS  0
#define LU_CPU_PPC   0
#define LU_CPU_RISCV 0
#define LU_CPU_X86   0

// C Runtime
#define LU_CRT_BIONIC 0
#define LU_CRT_BSD    0
#define LU_CRT_GLIBC  0
#define LU_CRT_LIBCXX 0
#define LU_CRT_MINGW  0
#define LU_CRT_MSVC   0
#define LU_CRT_NEWLIB 0

#ifndef LU_CRT_NONE
#	define LU_CRT_NONE 0
#endif // LU_CRT_NONE

#define LU_DEBUG 0
#define LU_RELEASE 0

// Language standard version
#define LU_LANGUAGE_CPP14 201402L
#define LU_LANGUAGE_CPP17 201703L
#define LU_LANGUAGE_CPP20 202002L
#define LU_LANGUAGE_CPP23 202207L

// Platform
#define LU_PLATFORM_ANDROID    0
#define LU_PLATFORM_BSD        0
#define LU_PLATFORM_EMSCRIPTEN 0
#define LU_PLATFORM_HAIKU      0
#define LU_PLATFORM_HURD       0
#define LU_PLATFORM_IOS        0
#define LU_PLATFORM_LINUX      0
#define LU_PLATFORM_NX         0
#define LU_PLATFORM_OSX        0
#define LU_PLATFORM_PS4        0
#define LU_PLATFORM_PS5        0
#define LU_PLATFORM_RPI        0
#define LU_PLATFORM_WINDOWS    0
#define LU_PLATFORM_WINRT      0
#define LU_PLATFORM_XBOXONE    0

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(__clang__)
// clang defines __GNUC__ or _MSC_VER
#	undef  LU_COMPILER_CLANG
#	define LU_COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#	if defined(__clang_analyzer__)
#		undef  LU_COMPILER_CLANG_ANALYZER
#		define LU_COMPILER_CLANG_ANALYZER 1
#	endif // defined(__clang_analyzer__)
#elif defined(_MSC_VER)
#	undef  LU_COMPILER_MSVC
#	define LU_COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
#	undef  LU_COMPILER_GCC
#	define LU_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#	error "LU_COMPILER_* is not defined!"
#endif //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__)     \
 || defined(__aarch64__) \
 || defined(_M_ARM)
#	undef  LU_CPU_ARM
#	define LU_CPU_ARM 1
#	define LU_CACHE_LINE_SIZE 64
#elif defined(__MIPSEL__)     \
 ||   defined(__mips_isa_rev) \
 ||   defined(__mips64)
#	undef  LU_CPU_MIPS
#	define LU_CPU_MIPS 1
#	define LU_CACHE_LINE_SIZE 64
#elif defined(_M_PPC)        \
 ||   defined(__powerpc__)   \
 ||   defined(__powerpc64__)
#	undef  LU_CPU_PPC
#	define LU_CPU_PPC 1
#	define LU_CACHE_LINE_SIZE 128
#elif defined(__riscv)   \
 ||   defined(__riscv__) \
 ||   defined(RISCVEL)
#	undef  LU_CPU_RISCV
#	define LU_CPU_RISCV 1
#	define LU_CACHE_LINE_SIZE 64
#elif defined(_M_IX86)    \
 ||   defined(_M_X64)     \
 ||   defined(__i386__)   \
 ||   defined(__x86_64__)
#	undef  LU_CPU_X86
#	define LU_CPU_X86 1
#	define LU_CACHE_LINE_SIZE 64
#else // PNaCl doesn't have CPU defined.
#	undef  LU_CPU_JIT
#	define LU_CPU_JIT 1
#	define LU_CACHE_LINE_SIZE 64
#endif //

#if defined(__x86_64__)    \
 || defined(_M_X64)        \
 || defined(__aarch64__)   \
 || defined(__64BIT__)     \
 || defined(__mips64)      \
 || defined(__powerpc64__) \
 || defined(__ppc64__)     \
 || defined(__LP64__)
#	undef  LU_ARCH_64BIT
#	define LU_ARCH_64BIT 64
#else
#	undef  LU_ARCH_32BIT
#	define LU_ARCH_32BIT 32
#endif //

#if LU_CPU_PPC
// __BIG_ENDIAN__ is gcc predefined macro
#	if defined(__BIG_ENDIAN__)
#		undef  LU_CPU_ENDIAN_BIG
#		define LU_CPU_ENDIAN_BIG 1
#	else
#		undef  LU_CPU_ENDIAN_LITTLE
#		define LU_CPU_ENDIAN_LITTLE 1
#	endif
#else
#	undef  LU_CPU_ENDIAN_LITTLE
#	define LU_CPU_ENDIAN_LITTLE 1
#endif // LU_CPU_PPC

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_DURANGO) || defined(_XBOX_ONE)
#	undef  LU_PLATFORM_XBOXONE
#	define LU_PLATFORM_XBOXONE 1
#elif defined(_WIN32) || defined(_WIN64)
// http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif // NOMINMAX
//  If _USING_V110_SDK71_ is defined it means we are using the v110_xp or v120_xp toolset.
#	if defined(_MSC_VER) && (_MSC_VER >= 1700) && (!_USING_V110_SDK71_)
#		include <winapifamily.h>
#	endif // defined(_MSC_VER) && (_MSC_VER >= 1700) && (!_USING_V110_SDK71_)
#	if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#		undef  LU_PLATFORM_WINDOWS
#		if !defined(WINVER) && !defined(_WIN32_WINNT)
#			if LU_ARCH_64BIT
//				When building 64-bit target Win7 and above.
#				define WINVER 0x0601
#				define _WIN32_WINNT 0x0601
#			else
//				Windows Server 2003 with SP1, Windows XP with SP2 and above
#				define WINVER 0x0502
#				define _WIN32_WINNT 0x0502
#			endif // LU_ARCH_64BIT
#		endif // !defined(WINVER) && !defined(_WIN32_WINNT)
#		define LU_PLATFORM_WINDOWS _WIN32_WINNT
#	else
#		undef  LU_PLATFORM_WINRT
#		define LU_PLATFORM_WINRT 1
#	endif
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#	include <sys/cdefs.h> // Defines __BIONIC__ and includes android/api-level.h
#	undef  LU_PLATFORM_ANDROID
#	define LU_PLATFORM_ANDROID __ANDROID_API__
#elif defined(__VCCOREVER__)
// RaspberryPi compiler defines __linux__
#	undef  LU_PLATFORM_RPI
#	define LU_PLATFORM_RPI 1
#elif  defined(__linux__)
#	undef  LU_PLATFORM_LINUX
#	define LU_PLATFORM_LINUX 1
#elif  defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) \
	|| defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#	undef  LU_PLATFORM_IOS
#	define LU_PLATFORM_IOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#	undef  LU_PLATFORM_OSX
#	define LU_PLATFORM_OSX __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#elif defined(__EMSCRIPTEN__)
#	undef  LU_PLATFORM_EMSCRIPTEN
#	define LU_PLATFORM_EMSCRIPTEN 1
#elif defined(__ORBIS__)
#	undef  LU_PLATFORM_PS4
#	define LU_PLATFORM_PS4 1
#elif defined(__PROSPERO__)
#	undef  LU_PLATFORM_PS5
#	define LU_PLATFORM_PS5 1
#elif  defined(__FreeBSD__)        \
	|| defined(__FreeBSD_kernel__) \
	|| defined(__NetBSD__)         \
	|| defined(__OpenBSD__)        \
	|| defined(__DragonFly__)
#	undef  LU_PLATFORM_BSD
#	define LU_PLATFORM_BSD 1
#elif defined(__GNU__)
#	undef  LU_PLATFORM_HURD
#	define LU_PLATFORM_HURD 1
#elif defined(__NX__)
#	undef  LU_PLATFORM_NX
#	define LU_PLATFORM_NX 1
#elif defined(__HAIKU__)
#	undef  LU_PLATFORM_HAIKU
#	define LU_PLATFORM_HAIKU 1
#endif //

#if !LU_CRT_NONE
// https://sourceforge.net/p/predef/wiki/Libraries/
#	if defined(__BIONIC__)
#		undef  LU_CRT_BIONIC
#		define LU_CRT_BIONIC 1
#	elif defined(_MSC_VER)
#		undef  LU_CRT_MSVC
#		define LU_CRT_MSVC 1
#	elif defined(__GLIBC__)
#		undef  LU_CRT_GLIBC
#		define LU_CRT_GLIBC (__GLIBC__ * 10000 + __GLIBC_MINOR__ * 100)
#	elif defined(__MINGW32__) || defined(__MINGW64__)
#		undef  LU_CRT_MINGW
#		define LU_CRT_MINGW 1
#	elif defined(__apple_build_version__) || defined(__ORBIS__) || defined(__EMSCRIPTEN__) || defined(__llvm__) || defined(__HAIKU__)
#		undef  LU_CRT_LIBCXX
#		define LU_CRT_LIBCXX 1
#	elif LU_PLATFORM_BSD
#		undef  LU_CRT_BSD
#		define LU_CRT_BSD 1
#	endif //

#	if !LU_CRT_BIONIC \
	&& !LU_CRT_BSD    \
	&& !LU_CRT_GLIBC  \
	&& !LU_CRT_LIBCXX \
	&& !LU_CRT_MINGW  \
	&& !LU_CRT_MSVC   \
	&& !LU_CRT_NEWLIB
#		undef  LU_CRT_NONE
#		define LU_CRT_NONE 1
#	endif // LU_CRT_*
#endif // !LU_CRT_NONE

///
#define LU_PLATFORM_POSIX (0   \
	||  LU_PLATFORM_ANDROID    \
	||  LU_PLATFORM_BSD        \
	||  LU_PLATFORM_EMSCRIPTEN \
	||  LU_PLATFORM_HAIKU      \
	||  LU_PLATFORM_HURD       \
	||  LU_PLATFORM_IOS        \
	||  LU_PLATFORM_LINUX      \
	||  LU_PLATFORM_NX         \
	||  LU_PLATFORM_OSX        \
	||  LU_PLATFORM_PS4        \
	||  LU_PLATFORM_PS5        \
	||  LU_PLATFORM_RPI        \
	)

///
#define LU_PLATFORM_NONE !(0   \
	||  LU_PLATFORM_ANDROID    \
	||  LU_PLATFORM_BSD        \
	||  LU_PLATFORM_EMSCRIPTEN \
	||  LU_PLATFORM_HAIKU      \
	||  LU_PLATFORM_HURD       \
	||  LU_PLATFORM_IOS        \
	||  LU_PLATFORM_LINUX      \
	||  LU_PLATFORM_NX         \
	||  LU_PLATFORM_OSX        \
	||  LU_PLATFORM_PS4        \
	||  LU_PLATFORM_PS5        \
	||  LU_PLATFORM_RPI        \
	||  LU_PLATFORM_WINDOWS    \
	||  LU_PLATFORM_WINRT      \
	||  LU_PLATFORM_XBOXONE    \
	)

///
#define LU_PLATFORM_OS_CONSOLE  (0 \
	||  LU_PLATFORM_NX             \
	||  LU_PLATFORM_PS4            \
	||  LU_PLATFORM_PS5            \
	||  LU_PLATFORM_WINRT          \
	||  LU_PLATFORM_XBOXONE        \
	)

///
#define LU_PLATFORM_OS_DESKTOP  (0 \
	||  LU_PLATFORM_BSD            \
	||  LU_PLATFORM_HAIKU          \
	||  LU_PLATFORM_HURD           \
	||  LU_PLATFORM_LINUX          \
	||  LU_PLATFORM_OSX            \
	||  LU_PLATFORM_WINDOWS        \
	)

///
#define LU_PLATFORM_OS_EMBEDDED (0 \
	||  LU_PLATFORM_RPI            \
	)

///
#define LU_PLATFORM_OS_MOBILE   (0 \
	||  LU_PLATFORM_ANDROID        \
	||  LU_PLATFORM_IOS            \
	)

///
#define LU_PLATFORM_OS_WEB      (0 \
	||  LU_PLATFORM_EMSCRIPTEN     \
	)

///
#if LU_COMPILER_GCC
#	define LU_COMPILER_NAME "GCC "       \
		LU_STRINGIZE(__GNUC__) "."       \
		LU_STRINGIZE(__GNUC_MINOR__) "." \
		LU_STRINGIZE(__GNUC_PATCHLEVEL__)
#elif LU_COMPILER_CLANG
#	define LU_COMPILER_NAME "Clang "      \
		LU_STRINGIZE(__clang_major__) "." \
		LU_STRINGIZE(__clang_minor__) "." \
		LU_STRINGIZE(__clang_patchlevel__)
#elif LU_COMPILER_MSVC
#	if LU_COMPILER_MSVC >= 1930 // Visual Studio 2022
#		define LU_COMPILER_NAME "MSVC 17.0"
#	elif LU_COMPILER_MSVC >= 1920 // Visual Studio 2019
#		define LU_COMPILER_NAME "MSVC 16.0"
#	elif LU_COMPILER_MSVC >= 1910 // Visual Studio 2017
#		define LU_COMPILER_NAME "MSVC 15.0"
#	elif LU_COMPILER_MSVC >= 1900 // Visual Studio 2015
#		define LU_COMPILER_NAME "MSVC 14.0"
#	elif LU_COMPILER_MSVC >= 1800 // Visual Studio 2013
#		define LU_COMPILER_NAME "MSVC 12.0"
#	elif LU_COMPILER_MSVC >= 1700 // Visual Studio 2012
#		define LU_COMPILER_NAME "MSVC 11.0"
#	elif LU_COMPILER_MSVC >= 1600 // Visual Studio 2010
#		define LU_COMPILER_NAME "MSVC 10.0"
#	elif LU_COMPILER_MSVC >= 1500 // Visual Studio 2008
#		define LU_COMPILER_NAME "MSVC 9.0"
#	else
#		define LU_COMPILER_NAME "MSVC"
#	endif //
#endif // LU_COMPILER_

#if LU_PLATFORM_ANDROID
#	define LU_PLATFORM_NAME "Android " \
				LU_STRINGIZE(LU_PLATFORM_ANDROID)
#elif LU_PLATFORM_BSD
#	define LU_PLATFORM_NAME "BSD"
#elif LU_PLATFORM_EMSCRIPTEN
#	define LU_PLATFORM_NAME "asm.js "          \
		LU_STRINGIZE(__EMSCRIPTEN_major__) "." \
		LU_STRINGIZE(__EMSCRIPTEN_minor__) "." \
		LU_STRINGIZE(__EMSCRIPTEN_tiny__)
#elif LU_PLATFORM_HAIKU
#	define LU_PLATFORM_NAME "Haiku"
#elif LU_PLATFORM_HURD
#	define LU_PLATFORM_NAME "Hurd"
#elif LU_PLATFORM_IOS
#	define LU_PLATFORM_NAME "iOS"
#elif LU_PLATFORM_LINUX
#	define LU_PLATFORM_NAME "Linux"
#elif LU_PLATFORM_NONE
#	define LU_PLATFORM_NAME "None"
#elif LU_PLATFORM_NX
#	define LU_PLATFORM_NAME "NX"
#elif LU_PLATFORM_OSX
#	define LU_PLATFORM_NAME "OSX"
#elif LU_PLATFORM_PS4
#	define LU_PLATFORM_NAME "PlayStation 4"
#elif LU_PLATFORM_PS5
#	define LU_PLATFORM_NAME "PlayStation 5"
#elif LU_PLATFORM_RPI
#	define LU_PLATFORM_NAME "RaspberryPi"
#elif LU_PLATFORM_WINDOWS
#	define LU_PLATFORM_NAME "Windows"
#elif LU_PLATFORM_WINRT
#	define LU_PLATFORM_NAME "WinRT"
#elif LU_PLATFORM_XBOXONE
#	define LU_PLATFORM_NAME "Xbox One"
#else
#	error "Unknown LU_PLATFORM!"
#endif // LU_PLATFORM_

#if LU_CPU_ARM
#	define LU_CPU_NAME "ARM"
#elif LU_CPU_JIT
#	define LU_CPU_NAME "JIT-VM"
#elif LU_CPU_MIPS
#	define LU_CPU_NAME "MIPS"
#elif LU_CPU_PPC
#	define LU_CPU_NAME "PowerPC"
#elif LU_CPU_RISCV
#	define LU_CPU_NAME "RISC-V"
#elif LU_CPU_X86
#	define LU_CPU_NAME "x86"
#endif // LU_CPU_

#if LU_CRT_BIONIC
#	define LU_CRT_NAME "Bionic libc"
#elif LU_CRT_BSD
#	define LU_CRT_NAME "BSD libc"
#elif LU_CRT_GLIBC
#	define LU_CRT_NAME "GNU C Library"
#elif LU_CRT_MSVC
#	define LU_CRT_NAME "MSVC C Runtime"
#elif LU_CRT_MINGW
#	define LU_CRT_NAME "MinGW C Runtime"
#elif LU_CRT_LIBCXX
#	define LU_CRT_NAME "Clang C Library"
#elif LU_CRT_NEWLIB
#	define LU_CRT_NAME "Newlib"
#elif LU_CRT_NONE
#	define LU_CRT_NAME "None"
#else
#	error "Unknown LU_CRT!"
#endif // LU_CRT_

#if LU_ARCH_32BIT
#	define LU_ARCH_NAME "32-bit"
#elif LU_ARCH_64BIT
#	define LU_ARCH_NAME "64-bit"
#endif // LU_ARCH_

#define LU_CPP_NAME "C++20"

#ifdef NDEBUG
#   undef LU_RELEASE
#   define LU_RELEASE 1
#else
#   undef LU_DEBUG
#   define LU_DEBUG 1
#endif

#if !LU_DEBUG && !LU_RELEASE
#   error "neither debug or release set"
#endif

#define LU_SHARED
#ifdef LU_SHARED
#   if LU_COMPILER_MSVC
#       define LU_API_EXPORT __declspec(dllexport)
#   else
#       define LU_API_EXPORT __attribute__((visibility("default")))
#   endif
#else
#   define LU_API_EXPORT
#endif

#if LU_COMPILER_MSVC
#   define LU_FLATTEN __forceinline
#   define LU_FORCEINLINE __forceinline
#   define LU_NOINLINE __declspec(noinline)
#   define LU_HOT
#   define LU_COLD
#else
#   define LU_FLATTEN __attribute__((flatten))
#   define LU_FORCEINLINE __attribute__((always_inline))
#   define LU_NOINLINE __attribute__((noinline))
#   define LU_HOT __attribute__((hot))
#   define LU_COLD __attribute__((cold))
#endif
