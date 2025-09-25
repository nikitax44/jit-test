#include "code.hpp"
#include <cstdint>
#include <sys/mman.h>

uint64_t Code::invoke(void *mem) const {
  typedef uint64_t (*fn)(void *);
  return ((fn)this->mem)(mem);
}
Code::~Code() { munmap(this->mem, this->size); }
