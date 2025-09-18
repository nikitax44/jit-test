#pragma once
#include <cstddef>
#include <cstdint>
class Code {
  void *mem;
  const std::size_t size;
  Code(void *mem, const std::size_t size) : mem(mem), size(size) {}

  friend class Assembler;

public:
  uint64_t invoke();
  ~Code();
};
