#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unistd.h>

void syscall_impl(uint32_t *buf) {
  uint32_t id = buf[8];
  uint8_t *memory = reinterpret_cast<uint8_t *>(buf) + 4 * 32;
  switch (id) {
  case 0:
    std::cerr << "  R0 =" << std::setw(5) << buf[0] << ", R1 =" << std::setw(5)
              << buf[1] << ", R2 =" << std::setw(5) << buf[2]
              << ", R3 =" << std::setw(5) << buf[3] << ", R4 =" << std::setw(5)
              << buf[4] << ", R5 =" << std::setw(5) << buf[5]
              << ", R6 =" << std::setw(5) << buf[6] << ", R7 =" << std::setw(5)
              << buf[7] << std::endl;

    std::cerr << "  R8 =" << std::setw(5) << buf[8] << ", R9 =" << std::setw(5)
              << buf[9] << ", R10=" << std::setw(5) << buf[10]
              << ", R11=" << std::setw(5) << buf[11] << ", R12=" << std::setw(5)
              << buf[12] << ", R13=" << std::setw(5) << buf[13]
              << ", R14=" << std::setw(5) << buf[14] << ", R15=" << std::setw(5)
              << buf[15] << std::endl;

    std::cerr << "  R16=" << std::setw(5) << buf[16] << ", R17=" << std::setw(5)
              << buf[17] << ", R18=" << std::setw(5) << buf[18]
              << ", R19=" << std::setw(5) << buf[19] << ", R20=" << std::setw(5)
              << buf[20] << ", R21=" << std::setw(5) << buf[21]
              << ", R22=" << std::setw(5) << buf[22] << ", R23=" << std::setw(5)
              << buf[23] << std::endl;

    std::cerr << "  R24=" << std::setw(5) << buf[24] << ", R25=" << std::setw(5)
              << buf[25] << ", R26=" << std::setw(5) << buf[26]
              << ", R27=" << std::setw(5) << buf[27] << ", R28=" << std::setw(5)
              << buf[18] << ", R29=" << std::setw(5) << buf[29]
              << ", R30=" << std::setw(5) << buf[30] << ", R31=" << std::setw(5)
              << buf[31] << std::endl;

    std::cerr << std::endl;
    break;
  case 1:
    std::exit(buf[0]);
  case 4:
    buf[0] = write(buf[0], &memory[buf[1]], buf[2]);
    break;
  default:
    std::cerr << "invalid syscall " << buf[8] << " was invoked" << std::endl;
    std::exit(42);
  }
}
