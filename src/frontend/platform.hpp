#pragma once

// OS
#if defined(_WIN64) || defined(__MINGW64__)
#  define PLAT_OS_WIN64 1
#elif defined(_WIN32) || defined(__MINGW32__)
// _WIN32 is also defined for WIN64
#  define PLAT_OS_WIN32 1
#endif

// CPU
#if defined(__i386__) || defined(_M_IX86)
#  define PLAT_CPU_X86 1
#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#  define PLAT_CPU_AMD64 1
#endif

// ABI
#if defined(PLAT_CPU_X86)
#  define ABI_CDECL 1
#elif defined(PLAT_CPU_AMD64)
#  if defined(PLAT_OS_WIN64)
#    define ABI_FASTCALL64 1
#  else
#    define ABI_SYSV 1
#  endif
#else
#  error "Unsupported architecture: only x86 or x86_64 are allowed"
#endif
