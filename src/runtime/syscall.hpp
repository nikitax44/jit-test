#include "../frontend/types.hpp"
#include <cstdint>

extern "C" uint64_t syscall_impl(Cpu &cpu, Memory &mem);

static void tailcall_nop(Cpu &cpu, Memory &mem, uint32_t Addr) {}
void tailcall_exit(Cpu &cpu, Memory &mem, uint32_t payload);

#define NOP_TAG 0
#define TAIL_NOP_TAG 1
#define EXIT_TAG 2
typedef void (*tailcall_handler)(Cpu &cpu, Memory &mem, uint32_t payload);
static tailcall_handler TABLE[2] = {tailcall_nop, tailcall_exit};

enum class Syscall : Reg {
  Syscall_Debug = 0,
  Syscall_Exit = 1,
  Syscall_Write = 4,
  Syscall_Put_uint32_t = 99,
  Syscall_Jmp = 100,
};
