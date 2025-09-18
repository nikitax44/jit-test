#include "code.hpp"
#include <cstdint>
#include <sys/mman.h>

uint64_t Code::invoke() {
  typedef uint64_t (*fn)(void);
  return ((fn)this->mem)();
}
Code::~Code() { munmap(this->mem, this->size); }
