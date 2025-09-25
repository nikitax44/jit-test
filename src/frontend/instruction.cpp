#include "instruction.hpp"
#include <cstdint>
#include <memory>

struct Insn_DUMMY {
  unsigned opcode : 6 = 0;
  unsigned dummy : 20;
  unsigned func : 6;

  Insn_DUMMY(uint32_t bits)
      : opcode(bits >> 26), dummy(bits >> 6), func(bits) {}
};

static std::unique_ptr<Insn> decode_func(uint32_t insn) {
  Insn_DUMMY dummy = insn;
  switch (dummy.func) {
  case 0b000100: // MOVZ
  case 0b011111: // SUB
  case 0b011010: // ADD
    return std::make_unique<Insn_D>(insn);

  case 0b000011: // SELC
  case 0b011101: // RBIT
    return std::make_unique<Insn_E>(insn);
  case 0b011001: // SYSCALL
    return std::make_unique<Insn_SYSCALL>(insn);
  default:
    return std::make_unique<Insn_ILLEGAL>(insn);
  }
}

std::unique_ptr<Insn> Insn::decode_insn(uint32_t insn) {
  Insn_DUMMY dummy = insn;
  switch (dummy.opcode) {
  case 0:
    return decode_func(insn);

  case 0b010011: // BEQ
  case 0b111011: // SLTI
    return std::make_unique<Insn_A>(insn);

  case 0b100010: // USAT
  case 0b111001: // STP
  case 0b001100: // RORI
    return std::make_unique<Insn_B>(insn);

  case 0b100011: // LD
  case 0b100101: // ST
    return std::make_unique<Insn_C>(insn);

  case 0b010110: // J
    return std::make_unique<Insn_J>(insn);

  default:
    return std::make_unique<Insn_ILLEGAL>(insn);
  }
}

std::string Insn_A::transpile(size_t PC) const {
  if (this->opcode == 0b010011) {
    // BEQ
    size_t target = this->jump_dest(PC).value();
    return std::format("mov eax, [rdi+{}];"
                       "mov ecx, [rdi+{}];"
                       "cmp eax, edx;"
                       "jz addr_{};",
                       this->rs, this->rt, target);
  } else {
    // SLTI
    int32_t imm = this->imm16;
    return std::format("mov eax, [rdi+{}];"
                       "cmp eax, {};"
                       "setl al;"
                       "movzx eax, al;"
                       "mov [rdi+{}], eax;",
                       this->rs, imm, this->rt);
  }
}

std::string Insn_B::transpile(size_t PC) const {
  if (this->opcode == 0b100010) {
    // USAT: clamp to n bits
    return "int3";
  } else if (this->opcode == 0b111001) {
    // STP
    return std::format("mov eax, [rdi+{}];" // base
                       "mov edx, [rdi+{}];" // rt1
                       "mov [rax+rdi+4*32+{}], edx;"
                       "mov edx, [rdi+{}];" // rt2
                       "mov [rax+rdi+4*32+{}], edx;",
                       this->rd, this->rs, this->imm11, this->imm5,
                       this->imm11 + 4);
  } else {
    // RORI
    return std::format("mov eax, [rdi+{}];"
                       "ror eax, {};"
                       "mov [rdi+{}], eax;",
                       this->rs, this->imm5, this->rd);
  }
}

std::string Insn_C::transpile(size_t PC) const {
  if (this->opcode == 0b100011) {
    // LD
    return std::format("mov eax, [rdi+{}];"
                       "mov rax, [rax+rdi+4*32+{}];"
                       "mov [rdi+{}], eax;",
                       this->base, this->offset, this->rt);
  } else {
    // ST
    return std::format("mov eax, [rdi+{}];"
                       "mov edx, [rdi+{}];"
                       "mov [rax+rdi+4*32+{}], edx;",
                       this->base, this->rt, this->offset);
  }
}
