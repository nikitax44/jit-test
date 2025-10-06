#ifdef __i386__
    .intel_syntax noprefix
    .globl  cdecl2sysv32
    .type   cdecl2sysv32, @function
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
