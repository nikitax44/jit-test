#include "stripe.hpp"
#include "../runtime/syscall.hpp"
#include "platform.hpp" // IWYU pragma: keep
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

#ifdef ABI_CDECL
extern "C" uint64_t cdecl2sysv32(Func func, Cpu &, Memory &);
#endif

Stripe::Stripe(std::span<InsnWrap> insns, Addr start_PC, Addr end_PC)
    : PC2offset(), start_PC(start_PC), end_PC(end_PC), rt() {
  assert(end_PC < insns.size() * sizeof(InsnWrap));
  auto branch = insns[end_PC / sizeof(InsnWrap)];

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

void Stripe::invoke(Cpu &cpu, Memory &mem) const {
  if (!this->contains(cpu.pc)) {
    throw std::runtime_error(
        std::format("invoke: pc out of stripe: pc={}", cpu.pc));
  }
  do {
    size_t idx = (cpu.pc - start_PC) / sizeof(InsnWrap);
    assert(idx < this->PC2offset.size());
    size_t off = this->PC2offset[idx];
    Func func = (Func)((size_t)fn + off);

#ifdef ABI_CDECL
    // cdecl passes arguments via the stack so it would be way slower to load
    // REG_CPU and REG_MEM on each instruction.
    //
    // cdecl2sysv32 saves the edi,esi to the stack, overwrites them with cpu,mem
    // and then jumps into the middle of the stripe.
    uint64_t res = cdecl2sysv32(func, cpu, mem);
#else
    uint64_t res = func(cpu, mem);
#endif

    if ((uint32_t)res != 0) {
      TABLE[(uint32_t)res - 1](cpu, mem, res >> 32);
    }
  } while (this->contains(cpu.pc));
}
