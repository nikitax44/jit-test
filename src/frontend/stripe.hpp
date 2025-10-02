#pragma once
#include "types.hpp"
#include <asmjit/core/globals.h>
#include <asmjit/core/jitruntime.h>
#include <asmjit/x86/x86assembler.h>
#include <cstddef>
#include <map>

typedef Addr (*Func)(void *mem);

class Stripe {
  std::map<Addr, size_t> PC2offset;
  Addr start_PC;
  asmjit::JitRuntime rt;
  void *fn;
  // TODO: store weak_ptrs to two possible destination Stripes
  std::optional<Addr> const_next;

public:
  Stripe(const Stripe &) = delete;
  Stripe &operator=(const Stripe &) = delete;

  Stripe(const uint32_t *insns, Addr start_PC, Addr end_PC);
  ~Stripe() { this->rt.release(fn); }

  Addr startPC() const { return this->start_PC; }

  Addr invoke(Addr PC, void *mem) const;
};
