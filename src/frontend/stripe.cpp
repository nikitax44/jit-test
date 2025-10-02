#include "stripe.hpp"
#include "types.hpp"
#include <asmjit/core/operand.h>
#include <cassert>
#include <span>
#include <stdexcept>

Stripe::Stripe(std::span<InsnWrap> insns, Addr start_PC, Addr end_PC)
    : PC2offset(), start_PC(start_PC), end_PC(end_PC), rt() {
  assert(end_PC / 4 < insns.size());
  auto branch = insns[end_PC / 4];
  this->const_next = branch.const_jump(end_PC);

  asmjit::CodeHolder code;
  code.init(rt.environment(), rt.cpuFeatures());

  asmjit::x86::Assembler a(&code);
  PC2offset.resize((end_PC - start_PC) / 4 + 1);

  for (size_t PC = start_PC; PC <= end_PC; PC += 4) {
    PC2offset[(PC - start_PC) / 4] = a.offset(); // faster than push_back

    auto insn = insns[PC / 4];
    insn.transpile(a, PC);
  }
  a.ud2(); // just to be safe
  auto err = rt.add(&fn, &code);

  if (err != asmjit::kErrorOk) {
    throw std::runtime_error(std::string("Failed to assemble: ") +
                             asmjit::DebugUtils::errorAsString(err));
  }
}

#if !defined(__x86_64__) && !defined(_M_X64)
#error "This code requires x86-64 architecture"
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) ||              \
    defined(__MINGW64__) || defined(_MSC_VER)
#error "System V x86_64 ABI required: this code will break on Windows targets";
#endif

Addr Stripe::invoke(Addr PC, void *mem) const {
  size_t idx = (PC - start_PC) / 4;
  assert(idx < this->PC2offset.size());
  size_t off = this->PC2offset[idx];
  Func func = (Func)((size_t)fn + off);
  return func(mem);
}
