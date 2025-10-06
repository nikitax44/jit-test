#ifdef __i386__

#if defined(_WIN32) || defined(__MINGW32__)
#define cdecl2sysv32 _cdecl2sysv32
#endif

    .intel_syntax noprefix
    .globl  cdecl2sysv32
#if defined(_WIN32) || defined(__MINGW32__)
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

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(_MSC_VER)
    .section .note.GNU-stack,"",@progbits
#endif
