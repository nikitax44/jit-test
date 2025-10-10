#include "instructions.hpp"

#define decode_func(method, ...)                                               \
  switch (OpcodeFunc(this->bits & 0b111111)) {                                 \
  case OpcodeFunc::MOVZ:                                                       \
  case OpcodeFunc::SUB:                                                        \
  case OpcodeFunc::ADD:                                                        \
    return Insn_D(this->bits).method(__VA_ARGS__);                             \
                                                                               \
  case OpcodeFunc::SELC:                                                       \
  case OpcodeFunc::RBIT:                                                       \
    return Insn_E(this->bits).method(__VA_ARGS__);                             \
  case OpcodeFunc::SYSCALL:                                                    \
    return Insn_SYSCALL(this->bits).method(__VA_ARGS__);                       \
  default:                                                                     \
    return Insn_ILLEGAL(this->bits).method(__VA_ARGS__);                       \
  }

#define decode_insn(method, ...)                                               \
  switch (this->opcode()) {                                                    \
  case Opcode::FUNC: {                                                         \
    decode_func(method, __VA_ARGS__);                                          \
  } break;                                                                     \
                                                                               \
  case Opcode::BEQ:                                                            \
  case Opcode::SLTI:                                                           \
    return Insn_A(this->bits).method(__VA_ARGS__);                             \
                                                                               \
  case Opcode::USAT:                                                           \
  case Opcode::STP:                                                            \
  case Opcode::RORI:                                                           \
    return Insn_B(this->bits).method(__VA_ARGS__);                             \
                                                                               \
  case Opcode::LD:                                                             \
  case Opcode::ST:                                                             \
    return Insn_C(this->bits).method(__VA_ARGS__);                             \
                                                                               \
  case Opcode::J:                                                              \
    return Insn_J(this->bits).method(__VA_ARGS__);                             \
                                                                               \
  default:                                                                     \
    return Insn_ILLEGAL(this->bits).method(__VA_ARGS__);                       \
  }

void InsnWrap::transpile(asmjit::x86::Assembler &a, Addr pc) const {
  decode_insn(transpile, a, pc);
}

std::optional<Addr> InsnWrap::interpret(Cpu &cpu, Memory &mem) const {
  decode_insn(interpret, cpu, mem);
}
