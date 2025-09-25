#pragma once
#include <cstddef>
#include <cstdint>
class Code {
  void *mem;
  const std::size_t size;
  Code(void *mem, const std::size_t size) : mem(mem), size(size) {}

  friend class Assembler;

public:
  void *ptr() const { return mem; }
  uint64_t invoke(void *mem) const;
  ~Code();
};
