#include "platform.hpp"

#ifdef ABI_CDECL

#if PLAT_OS_WIN32
#define cdecl2sysv32 _cdecl2sysv32
#endif

    .intel_syntax noprefix
    .globl  cdecl2sysv32
#if PLAT_OS_WIN32
    .def    cdecl2sysv32; .scl 2; .type 32; .endef
#else
    .type   cdecl2sysv32, @function
#endif

cdecl2sysv32:
    push    edi
    push    esi

    mov     edi, DWORD PTR [esp+12+4*1]
    mov     esi, DWORD PTR [esp+12+4*2]

    call    DWORD PTR [esp+12+4*0]

    pop     esi
    pop     edi

    ret
#endif
