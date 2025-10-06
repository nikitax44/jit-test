#include "exit.hpp"
#include "syscall.hpp"

void tailcall_exit(Cpu &cpu, Memory &, uint32_t status) {
  throw exit_exception(status);
}
