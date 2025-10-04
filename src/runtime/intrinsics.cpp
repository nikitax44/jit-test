#include "exit.hpp"
#include "syscall.hpp"

Addr tailcall_exit(Memory &, uint32_t status) { throw exit_exception(status); }
Addr tailcall_jmp(Memory &, uint32_t dest) { return dest; }
