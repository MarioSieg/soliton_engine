// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#define STRINGIZE_IMPL(x) #x
#define STRINGIZE(x) STRINGIZE_IMPL(x)

// Architecture
#define ARCH_32BIT 0
#define ARCH_64BIT 0

// Compiler
#define COMPILER_CLANG          0
#define COMPILER_CLANG_ANALYZER 0
#define COMPILER_GCC            0
#define COMPILER_MSVC           0

// Endianness
#define CPU_ENDIAN_BIG    0
#define CPU_ENDIAN_LITTLE 0

// CPU
#define CPU_ARM   0
#define CPU_JIT   0
#define CPU_MIPS  0
#define CPU_PPC   0
#define CPU_RISCV 0
#define CPU_X86   0

// C Runtime
#define CRT_BIONIC 0
#define CRT_BSD    0
#define CRT_GLIBC  0
#define CRT_LIBCXX 0
#define CRT_MINGW  0
#define CRT_MSVC   0
#define CRT_NEWLIB 0

#ifndef CRT_NONE
#	define CRT_NONE 0
#endif // CRT_NONE

#define DEBUG 0
#define RELEASE 0

// Language standard version
#define LANGUAGE_CPP14 201402L
#define LANGUAGE_CPP17 201703L
#define LANGUAGE_CPP20 202002L
#define LANGUAGE_CPP23 202207L

// Platform
#define PLATFORM_ANDROID    0
#define PLATFORM_BSD        0
#define PLATFORM_EMSCRIPTEN 0
#define PLATFORM_HAIKU      0
#define PLATFORM_HURD       0
#define PLATFORM_IOS        0
#define PLATFORM_LINUX      0
#define PLATFORM_NX         0
#define PLATFORM_OSX        0
#define PLATFORM_PS4        0
#define PLATFORM_PS5        0
#define PLATFORM_RPI        0
#define PLATFORM_WINDOWS    0
#define PLATFORM_WINRT      0
#define PLATFORM_XBOXONE    0

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(__clang__)
// clang defines __GNUC__ or _MSC_VER
#	undef  COMPILER_CLANG
#	define COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#	if defined(__clang_analyzer__)
#		undef  COMPILER_CLANG_ANALYZER
#		define COMPILER_CLANG_ANALYZER 1
#	endif // defined(__clang_analyzer__)
#elif defined(_MSC_VER)
#	undef  COMPILER_MSVC
#	define COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
#	undef  COMPILER_GCC
#	define COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#	error "COMPILER_* is not defined!"
#endif //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__)     \
 || defined(__aarch64__) \
 || defined(_M_ARM)
#	undef  CPU_ARM
#	define CPU_ARM 1
#	define CACHE_LINE_SIZE 64
#elif defined(__MIPSEL__)     \
 ||   defined(__mips_isa_rev) \
 ||   defined(__mips64)
#	undef  CPU_MIPS
#	define CPU_MIPS 1
#	define CACHE_LINE_SIZE 64
#elif defined(_M_PPC)        \
 ||   defined(__powerpc__)   \
 ||   defined(__powerpc64__)
#	undef  CPU_PPC
#	define CPU_PPC 1
#	define CACHE_LINE_SIZE 128
#elif defined(__riscv)   \
 ||   defined(__riscv__) \
 ||   defined(RISCVEL)
#	undef  CPU_RISCV
#	define CPU_RISCV 1
#	define CACHE_LINE_SIZE 64
#elif defined(_M_IX86)    \
 ||   defined(_M_X64)     \
 ||   defined(__i386__)   \
 ||   defined(__x86_64__)
#	undef  CPU_X86
#	define CPU_X86 1
#	define CACHE_LINE_SIZE 64
#else // PNaCl doesn't have CPU defined.
#	undef  CPU_JIT
#	define CPU_JIT 1
#	define CACHE_LINE_SIZE 64
#endif //

#if defined(__x86_64__)    \
 || defined(_M_X64)        \
 || defined(__aarch64__)   \
 || defined(__64BIT__)     \
 || defined(__mips64)      \
 || defined(__powerpc64__) \
 || defined(__ppc64__)     \
 || defined(__LP64__)
#	undef  ARCH_64BIT
#	define ARCH_64BIT 64
#else
#	undef  ARCH_32BIT
#	define ARCH_32BIT 32
#endif //

#if CPU_PPC
// __BIG_ENDIAN__ is gcc predefined macro
#	if defined(__BIG_ENDIAN__)
#		undef  CPU_ENDIAN_BIG
#		define CPU_ENDIAN_BIG 1
#	else
#		undef  CPU_ENDIAN_LITTLE
#		define CPU_ENDIAN_LITTLE 1
#	endif
#else
#	undef  CPU_ENDIAN_LITTLE
#	define CPU_ENDIAN_LITTLE 1
#endif // CPU_PPC

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_DURANGO) || defined(_XBOX_ONE)
#	undef  PLATFORM_XBOXONE
#	define PLATFORM_XBOXONE 1
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
#		undef  PLATFORM_WINDOWS
#		if !defined(WINVER) && !defined(_WIN32_WINNT)
#			if ARCH_64BIT
//				When building 64-bit target Win7 and above.
#				define WINVER 0x0601
#				define _WIN32_WINNT 0x0601
#			else
//				Windows Server 2003 with SP1, Windows XP with SP2 and above
#				define WINVER 0x0502
#				define _WIN32_WINNT 0x0502
#			endif // ARCH_64BIT
#		endif // !defined(WINVER) && !defined(_WIN32_WINNT)
#		define PLATFORM_WINDOWS _WIN32_WINNT
#	else
#		undef  PLATFORM_WINRT
#		define PLATFORM_WINRT 1
#	endif
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#	include <sys/cdefs.h> // Defines __BIONIC__ and includes android/api-level.h
#	undef  PLATFORM_ANDROID
#	define PLATFORM_ANDROID __ANDROID_API__
#elif defined(__VCCOREVER__)
// RaspberryPi compiler defines __linux__
#	undef  PLATFORM_RPI
#	define PLATFORM_RPI 1
#elif  defined(__linux__)
#	undef  PLATFORM_LINUX
#	define PLATFORM_LINUX 1
#elif  defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) \
	|| defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#	undef  PLATFORM_IOS
#	define PLATFORM_IOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#	undef  PLATFORM_OSX
#	define PLATFORM_OSX __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#elif defined(__EMSCRIPTEN__)
#	undef  PLATFORM_EMSCRIPTEN
#	define PLATFORM_EMSCRIPTEN 1
#elif defined(__ORBIS__)
#	undef  PLATFORM_PS4
#	define PLATFORM_PS4 1
#elif defined(__PROSPERO__)
#	undef  PLATFORM_PS5
#	define PLATFORM_PS5 1
#elif  defined(__FreeBSD__)        \
	|| defined(__FreeBSD_kernel__) \
	|| defined(__NetBSD__)         \
	|| defined(__OpenBSD__)        \
	|| defined(__DragonFly__)
#	undef  PLATFORM_BSD
#	define PLATFORM_BSD 1
#elif defined(__GNU__)
#	undef  PLATFORM_HURD
#	define PLATFORM_HURD 1
#elif defined(__NX__)
#	undef  PLATFORM_NX
#	define PLATFORM_NX 1
#elif defined(__HAIKU__)
#	undef  PLATFORM_HAIKU
#	define PLATFORM_HAIKU 1
#endif //

#if !CRT_NONE
// https://sourceforge.net/p/predef/wiki/Libraries/
#	if defined(__BIONIC__)
#		undef  CRT_BIONIC
#		define CRT_BIONIC 1
#	elif defined(_MSC_VER)
#		undef  CRT_MSVC
#		define CRT_MSVC 1
#	elif defined(__GLIBC__)
#		undef  CRT_GLIBC
#		define CRT_GLIBC (__GLIBC__ * 10000 + __GLIBC_MINOR__ * 100)
#	elif defined(__MINGW32__) || defined(__MINGW64__)
#		undef  CRT_MINGW
#		define CRT_MINGW 1
#	elif defined(__apple_build_version__) || defined(__ORBIS__) || defined(__EMSCRIPTEN__) || defined(__llvm__) || defined(__HAIKU__)
#		undef  CRT_LIBCXX
#		define CRT_LIBCXX 1
#	elif PLATFORM_BSD
#		undef  CRT_BSD
#		define CRT_BSD 1
#	endif //

#	if !CRT_BIONIC \
	&& !CRT_BSD    \
	&& !CRT_GLIBC  \
	&& !CRT_LIBCXX \
	&& !CRT_MINGW  \
	&& !CRT_MSVC   \
	&& !CRT_NEWLIB
#		undef  CRT_NONE
#		define CRT_NONE 1
#	endif // CRT_*
#endif // !CRT_NONE

///
#define PLATFORM_POSIX (0   \
	||  PLATFORM_ANDROID    \
	||  PLATFORM_BSD        \
	||  PLATFORM_EMSCRIPTEN \
	||  PLATFORM_HAIKU      \
	||  PLATFORM_HURD       \
	||  PLATFORM_IOS        \
	||  PLATFORM_LINUX      \
	||  PLATFORM_NX         \
	||  PLATFORM_OSX        \
	||  PLATFORM_PS4        \
	||  PLATFORM_PS5        \
	||  PLATFORM_RPI        \
	)

///
#define PLATFORM_NONE !(0   \
	||  PLATFORM_ANDROID    \
	||  PLATFORM_BSD        \
	||  PLATFORM_EMSCRIPTEN \
	||  PLATFORM_HAIKU      \
	||  PLATFORM_HURD       \
	||  PLATFORM_IOS        \
	||  PLATFORM_LINUX      \
	||  PLATFORM_NX         \
	||  PLATFORM_OSX        \
	||  PLATFORM_PS4        \
	||  PLATFORM_PS5        \
	||  PLATFORM_RPI        \
	||  PLATFORM_WINDOWS    \
	||  PLATFORM_WINRT      \
	||  PLATFORM_XBOXONE    \
	)

///
#define PLATFORM_OS_CONSOLE  (0 \
	||  PLATFORM_NX             \
	||  PLATFORM_PS4            \
	||  PLATFORM_PS5            \
	||  PLATFORM_WINRT          \
	||  PLATFORM_XBOXONE        \
	)

///
#define PLATFORM_OS_DESKTOP  (0 \
	||  PLATFORM_BSD            \
	||  PLATFORM_HAIKU          \
	||  PLATFORM_HURD           \
	||  PLATFORM_LINUX          \
	||  PLATFORM_OSX            \
	||  PLATFORM_WINDOWS        \
	)

///
#define PLATFORM_OS_EMBEDDED (0 \
	||  PLATFORM_RPI            \
	)

///
#define PLATFORM_OS_MOBILE   (0 \
	||  PLATFORM_ANDROID        \
	||  PLATFORM_IOS            \
	)

///
#define PLATFORM_OS_WEB      (0 \
	||  PLATFORM_EMSCRIPTEN     \
	)

///
#if COMPILER_GCC
#	define COMPILER_NAME "GCC "       \
		STRINGIZE(__GNUC__) "."       \
		STRINGIZE(__GNUC_MINOR__) "." \
		STRINGIZE(__GNUC_PATCHLEVEL__)
#elif COMPILER_CLANG
#	define COMPILER_NAME "Clang "      \
		STRINGIZE(__clang_major__) "." \
		STRINGIZE(__clang_minor__) "." \
		STRINGIZE(__clang_patchlevel__)
#elif COMPILER_MSVC
#	if COMPILER_MSVC >= 1930 // Visual Studio 2022
#		define COMPILER_NAME "MSVC 17.0"
#	elif COMPILER_MSVC >= 1920 // Visual Studio 2019
#		define COMPILER_NAME "MSVC 16.0"
#	elif COMPILER_MSVC >= 1910 // Visual Studio 2017
#		define COMPILER_NAME "MSVC 15.0"
#	elif COMPILER_MSVC >= 1900 // Visual Studio 2015
#		define COMPILER_NAME "MSVC 14.0"
#	elif COMPILER_MSVC >= 1800 // Visual Studio 2013
#		define COMPILER_NAME "MSVC 12.0"
#	elif COMPILER_MSVC >= 1700 // Visual Studio 2012
#		define COMPILER_NAME "MSVC 11.0"
#	elif COMPILER_MSVC >= 1600 // Visual Studio 2010
#		define COMPILER_NAME "MSVC 10.0"
#	elif COMPILER_MSVC >= 1500 // Visual Studio 2008
#		define COMPILER_NAME "MSVC 9.0"
#	else
#		define COMPILER_NAME "MSVC"
#	endif //
#endif // COMPILER_

#if PLATFORM_ANDROID
#	define PLATFORM_NAME "Android " \
				STRINGIZE(PLATFORM_ANDROID)
#elif PLATFORM_BSD
#	define PLATFORM_NAME "BSD"
#elif PLATFORM_EMSCRIPTEN
#	define PLATFORM_NAME "asm.js "          \
		STRINGIZE(__EMSCRIPTEN_major__) "." \
		STRINGIZE(__EMSCRIPTEN_minor__) "." \
		STRINGIZE(__EMSCRIPTEN_tiny__)
#elif PLATFORM_HAIKU
#	define PLATFORM_NAME "Haiku"
#elif PLATFORM_HURD
#	define PLATFORM_NAME "Hurd"
#elif PLATFORM_IOS
#	define PLATFORM_NAME "iOS"
#elif PLATFORM_LINUX
#	define PLATFORM_NAME "Linux"
#elif PLATFORM_NONE
#	define PLATFORM_NAME "None"
#elif PLATFORM_NX
#	define PLATFORM_NAME "NX"
#elif PLATFORM_OSX
#	define PLATFORM_NAME "OSX"
#elif PLATFORM_PS4
#	define PLATFORM_NAME "PlayStation 4"
#elif PLATFORM_PS5
#	define PLATFORM_NAME "PlayStation 5"
#elif PLATFORM_RPI
#	define PLATFORM_NAME "RaspberryPi"
#elif PLATFORM_WINDOWS
#	define PLATFORM_NAME "Windows"
#elif PLATFORM_WINRT
#	define PLATFORM_NAME "WinRT"
#elif PLATFORM_XBOXONE
#	define PLATFORM_NAME "Xbox One"
#else
#	error "Unknown PLATFORM!"
#endif // PLATFORM_

#if CPU_ARM
#	define CPU_NAME "ARM"
#elif CPU_JIT
#	define CPU_NAME "JIT-VM"
#elif CPU_MIPS
#	define CPU_NAME "MIPS"
#elif CPU_PPC
#	define CPU_NAME "PowerPC"
#elif CPU_RISCV
#	define CPU_NAME "RISC-V"
#elif CPU_X86
#	define CPU_NAME "x86"
#endif // CPU_

#if CRT_BIONIC
#	define CRT_NAME "Bionic libc"
#elif CRT_BSD
#	define CRT_NAME "BSD libc"
#elif CRT_GLIBC
#	define CRT_NAME "GNU C Library"
#elif CRT_MSVC
#	define CRT_NAME "MSVC C Runtime"
#elif CRT_MINGW
#	define CRT_NAME "MinGW C Runtime"
#elif CRT_LIBCXX
#	define CRT_NAME "Clang C Library"
#elif CRT_NEWLIB
#	define CRT_NAME "Newlib"
#elif CRT_NONE
#	define CRT_NAME "None"
#else
#	error "Unknown CRT!"
#endif // CRT_

#if ARCH_32BIT
#	define ARCH_NAME "32-bit"
#elif ARCH_64BIT
#	define ARCH_NAME "64-bit"
#endif // ARCH_

#define CPP_NAME "C++20"

#ifdef NDEBUG
#   undef RELEASE
#   define RELEASE 1
#else
#   undef DEBUG
#   define DEBUG 1
#endif

#if !DEBUG && !RELEASE
#   error "neither debug or release set"
#endif

#if COMPILER_MSVC
#define HOTPROC
#else
#define HOTPROC __attribute__((hot))
#endif
