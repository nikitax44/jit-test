#include "stripe.hpp"
#include "instruction.hpp"

Stripe::Stripe(const uint32_t *insns, size_t start_PC, size_t end_PC)
    : PC2offset(), start_PC(start_PC), rt() {
  asmjit::CodeHolder code;
  code.init(rt.environment(), rt.cpuFeatures());

  asmjit::x86::Assembler a(&code);
  for (size_t i = start_PC / 4; i <= end_PC / 4; ++i) {
    auto insn = Insn::decode_insn(insns[i]);
    PC2offset.emplace(i * 4, a.offset());
    insn->transpile(a, i * 4);
  }
  a.ud2(); // just to be safe
  auto err = rt.add(&fn, &code);

  if (err != asmjit::kErrorOk) {
    throw std::runtime_error(std::string("Failed to assemble: ") +
                             asmjit::DebugUtils::errorAsString(err));
  }
}

size_t Stripe::invoke(size_t PC, void *mem) const {
  size_t off = this->PC2offset.at(PC);
  Func func = (Func)((size_t)fn + off);
  return func(mem);
}
