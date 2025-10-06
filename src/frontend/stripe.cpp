#include "stripe.hpp"
#include "types.hpp"
#include <asmjit/core/errorhandler.h>
#include <asmjit/x86/x86assembler.h>
#include <cassert>
#include <format>
#include <span>
#include <stdexcept>

class ThrowingErrorHandler : public asmjit::ErrorHandler {
  void handleError(asmjit::Error err, const char *message,
                   asmjit::BaseEmitter *origin) override {
    throw std::logic_error(message);
  }
};

static ThrowingErrorHandler errorHandler = {};

Stripe::Stripe(std::span<InsnWrap> insns, Addr start_PC, Addr end_PC)
    : PC2offset(), start_PC(start_PC), end_PC(end_PC), rt() {
  assert(end_PC < insns.size() * sizeof(InsnWrap));
  auto branch = insns[end_PC / sizeof(InsnWrap)];
  this->const_next = branch.const_jump(end_PC);

  asmjit::CodeHolder code;
  code.init(rt.environment(), rt.cpuFeatures());

  asmjit::x86::Assembler a(&code);
  a.setErrorHandler(&errorHandler);

  PC2offset.resize((end_PC - start_PC) / sizeof(InsnWrap) + 1);

  for (size_t pc = start_PC; pc <= end_PC; pc += sizeof(InsnWrap)) {
    PC2offset[(pc - start_PC) / sizeof(InsnWrap)] =
        a.offset(); // faster than push_back

    auto insn = insns[pc / sizeof(InsnWrap)];
    insn.transpile(a, pc);
  }
  a.ud2(); // just to be safe
  auto err = rt.add(&fn, &code);

  if (err != asmjit::kErrorOk) {
    throw std::runtime_error(std::format(
        "Failed to assemble: {}", asmjit::DebugUtils::errorAsString(err)));
  }
}

Addr Stripe::invoke(Cpu &cpu, Memory &mem) const {
  if (!this->contains(cpu.pc)) {
    throw std::runtime_error(
        std::format("invoke: pc out of stripe: pc={}", cpu.pc));
  }
  do {
    size_t idx = (cpu.pc - start_PC) / sizeof(InsnWrap);
    assert(idx < this->PC2offset.size());
    size_t off = this->PC2offset[idx];
    Func func = (Func)((size_t)fn + off);
    func(cpu, mem);
  } while (this->contains(cpu.pc));
  return cpu.pc;
}
