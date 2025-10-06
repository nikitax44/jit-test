#pragma once
#include "types.hpp"
#include <asmjit/core/globals.h>
#include <asmjit/core/jitruntime.h>
#include <asmjit/x86/x86assembler.h>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

typedef void (*Func)(Cpu &cpu, Memory &mem);

class Stripe {
  std::vector<size_t> PC2offset;
  Addr start_PC;
  Addr end_PC;
  asmjit::JitRuntime rt;
  void *fn;
  // TODO: store weak_ptrs to two possible destination Stripes
  std::optional<Addr> const_next;

public:
  Stripe(const Stripe &) = delete;
  Stripe &operator=(const Stripe &) = delete;

  Stripe(std::span<InsnWrap> insns, Addr start_PC, Addr end_PC);
  ~Stripe() { this->rt.release(fn); }

  Addr startPC() const { return this->start_PC; }
  Addr endPC() const { return this->end_PC; }

  inline bool contains(Addr pc) const { return start_PC <= pc && pc <= end_PC; }

  Addr invoke(Cpu &cpu, Memory &mem) const;
  mutable std::optional<std::weak_ptr<Stripe>> next;
};
