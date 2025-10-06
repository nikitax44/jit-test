#pragma once

#if defined(__i386__) || defined(_M_IX86)
#define ABI_CDECL 1
#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)

#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) ||              \
    defined(__MINGW64__) || defined(_MSC_VER)
#define ABI_FASTCALL64 1
#else
#define ABI_SYSV 1
#endif

#else
#error "Unsupported architecture: only x86 or x86_64 are allowed"
#endif
