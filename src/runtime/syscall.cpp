#include "syscall.hpp"
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unistd.h>

#define NOP NOP_TAG
#define JMP(addr) ((((uint64_t)(addr)) << 32) | JMP_TAG)
#define EXIT(addr) ((((uint64_t)(addr)) << 32) | EXIT_TAG)

extern "C" uint64_t syscall_impl(Memory &mem, Addr PC) {
  uint32_t(&regs)[32] = mem.reg;
  uint32_t id = regs[8];
  uint8_t *memory = mem.memory;
  switch (id) {
  case 0:
    std::cerr << "  R0 =" << std::setw(5) << regs[0] //
              << ", R1 =" << std::setw(5) << regs[1] //
              << ", R2 =" << std::setw(5) << regs[2] //
              << ", R3 =" << std::setw(5) << regs[3] //
              << ", R4 =" << std::setw(5) << regs[4] //
              << ", R5 =" << std::setw(5) << regs[5] //
              << ", R6 =" << std::setw(5) << regs[6] //
              << ", R7 =" << std::setw(5) << regs[7] << std::endl;

    std::cerr << "  R8 =" << std::setw(5) << regs[8] //
              << ", R9 =" << std::setw(5) << regs[9] //
              << ", R10=" << std::setw(5) << regs[10]
              << ", R11=" << std::setw(5) << regs[11]
              << ", R12=" << std::setw(5) << regs[12]
              << ", R13=" << std::setw(5) << regs[13]
              << ", R14=" << std::setw(5) << regs[14]
              << ", R15=" << std::setw(5) << regs[15] << std::endl;

    std::cerr << "  R16=" << std::setw(5) << regs[16]
              << ", R17=" << std::setw(5) << regs[17]
              << ", R18=" << std::setw(5) << regs[18]
              << ", R19=" << std::setw(5) << regs[19]
              << ", R20=" << std::setw(5) << regs[20]
              << ", R21=" << std::setw(5) << regs[21]
              << ", R22=" << std::setw(5) << regs[22]
              << ", R23=" << std::setw(5) << regs[23] << std::endl;

    std::cerr << "  R24=" << std::setw(5) << regs[24]
              << ", R25=" << std::setw(5) << regs[25]
              << ", R26=" << std::setw(5) << regs[26]
              << ", R27=" << std::setw(5) << regs[27]
              << ", R28=" << std::setw(5) << regs[18]
              << ", R29=" << std::setw(5) << regs[29]
              << ", R30=" << std::setw(5) << regs[30]
              << ", R31=" << std::setw(5) << regs[31] << std::endl;

    std::cerr << std::endl;
    break;
  case 1:
    return EXIT(regs[0]);
  case 4:
    regs[0] = write(regs[0], &memory[regs[1]], regs[2]);
    break;
  case 99:
    std::cout << regs[0] << std::endl;
    break;
  case 100: {
    Addr ret = PC + 4;
    PC = regs[0];
    regs[0] = ret;
    return JMP(PC);
  } break;

  default:
    std::cerr << "invalid syscall " << regs[8] << " was invoked" << std::endl;
    std::abort();
  }

  return NOP;
}
