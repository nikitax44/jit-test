#include "instructions.hpp"

static void transpile_func(InsnWrap insn, asmjit::x86::Assembler &a, Addr pc) {
  switch (OpcodeFunc(insn.bits & 0b111111)) {
  case OpcodeFunc::MOVZ:
  case OpcodeFunc::SUB:
  case OpcodeFunc::ADD:
    return Insn_D(insn.bits).transpile(a, pc);

  case OpcodeFunc::SELC:
  case OpcodeFunc::RBIT:
    return Insn_E(insn.bits).transpile(a, pc);
  case OpcodeFunc::SYSCALL:
    return Insn_SYSCALL(insn.bits).transpile(a, pc);
  default:
    return Insn_ILLEGAL(insn.bits).transpile(a, pc);
  }
}

static void transpile_insn(InsnWrap insn, asmjit::x86::Assembler &a, Addr pc) {
  switch (insn.opcode()) {
  case Opcode::FUNC:
    return transpile_func(insn, a, pc);

  case Opcode::BEQ:
  case Opcode::SLTI:
    return Insn_A(insn.bits).transpile(a, pc);

  case Opcode::USAT:
  case Opcode::STP:
  case Opcode::RORI:
    return Insn_B(insn.bits).transpile(a, pc);

  case Opcode::LD:
  case Opcode::ST:
    return Insn_C(insn.bits).transpile(a, pc);

  case Opcode::J:
    return Insn_J(insn.bits).transpile(a, pc);

  default:
    return Insn_ILLEGAL(insn.bits).transpile(a, pc);
  }
}

void InsnWrap::transpile(asmjit::x86::Assembler &a, Addr pc) const {
  return transpile_insn(*this, a, pc);
}
