#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

void syscall_impl(uint32_t *buf) {
  uint32_t id = buf[8];
  uint8_t *memory = reinterpret_cast<uint8_t *>(buf) + 4 * 32;
  switch (id) {
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
