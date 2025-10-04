#include "../frontend/types.hpp"

extern "C" uint64_t syscall_impl(Memory &mem, Addr PC);

Addr tailcall_jmp(Memory &mem, uint32_t Addr);
Addr tailcall_exit(Memory &mem, uint32_t payload);

#define NOP_TAG 0
#define JMP_TAG 1
#define EXIT_TAG 2
typedef Addr (*tailcall_handler)(Memory &mem, uint32_t payload);
static tailcall_handler TABLE[2] = {tailcall_jmp, tailcall_exit};
