#include "stripe.hpp"
#include <stdexcept>

Stripe::Stripe(const uint32_t *insns, Addr start_PC, Addr end_PC)
    : PC2offset(), start_PC(start_PC), rt() {
  asmjit::CodeHolder code;
  code.init(rt.environment(), rt.cpuFeatures());

  asmjit::x86::Assembler a(&code);
  for (size_t i = start_PC / 4; i <= end_PC / 4; ++i) {
    auto insn = std::bit_cast<InsnWrap>(insns[i]);
    PC2offset.emplace(i * 4, a.offset());
    insn.transpile(a, i * 4);
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
  size_t off = this->PC2offset.at(PC);
  Func func = (Func)((size_t)fn + off);
  return func(mem);
}
