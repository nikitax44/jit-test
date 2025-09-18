#include "asm.hpp"
#include "keystone/keystone.h"
#include <cstring>
#include <sys/mman.h>
#include <system_error>

Assembler::Assembler() {
  ks_err err = ks_open(KS_ARCH_X86, KS_MODE_64, &ks);
  if (err != KS_ERR_OK) {
    throw std::runtime_error(std::string("Failed to initialize Keystone: ") +
                             ks_strerror(err));
  }
}

Code Assembler::assemble(const char *code) {
  unsigned char *encoding;
  size_t size;
  size_t count;
  if (ks_asm(ks, (code + std::string("ud2")).c_str(), 0, &encoding, &size,
             &count) != KS_ERR_OK) {
    throw std::runtime_error(std::string("Failed to assemble: ") +
                             ks_strerror(ks_errno(ks)));
  }
  void *mem = static_cast<unsigned char *>(
      mmap(0, size, PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0));
  if (mem == MAP_FAILED) {
    ks_free(encoding);
    throw std::system_error();
  }
  std::memcpy(mem, encoding, size);
  ks_free(encoding);
  if (mprotect(mem, size, PROT_EXEC) != 0) {
    throw std::system_error();
  }

  return {mem, size};
}

Assembler::~Assembler() { ks_close(this->ks); }
