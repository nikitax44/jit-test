#pragma once
#include <asmjit/core/globals.h>
#include <asmjit/core/jitruntime.h>
#include <asmjit/x86/x86assembler.h>
#include <cstddef>
#include <map>

typedef size_t (*Func)(void *mem);

class Stripe {
  std::map<size_t, size_t> PC2offset;
  size_t start_PC;
  asmjit::JitRuntime rt;
  void *fn;

public:
  Stripe(const Stripe &) = delete;
  Stripe &operator=(const Stripe &) = delete;

  Stripe(const uint32_t *insns, size_t start_PC, size_t end_PC);
  ~Stripe() { this->rt.release(fn); }

  size_t startPC() const { return this->start_PC; }

  size_t invoke(size_t PC, void *mem) const;
};
