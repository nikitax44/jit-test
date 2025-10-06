#include "../frontend/types.hpp"
#include <cstdint>

extern "C" uint64_t syscall_impl(Memory &mem, Addr PC);

Addr tailcall_jmp(Memory &mem, uint32_t Addr);
Addr tailcall_exit(Memory &mem, uint32_t payload);

#define NOP_TAG 0
#define JMP_TAG 1
#define EXIT_TAG 2
typedef Addr (*tailcall_handler)(Memory &mem, uint32_t payload);
static tailcall_handler TABLE[2] = {tailcall_jmp, tailcall_exit};

enum class Syscall : Reg {
  Syscall_Debug = 0,
  Syscall_Exit = 1,
  Syscall_Write = 4,
  Syscall_Put_uint32_t = 99,
  Syscall_Jmp = 100,
};
