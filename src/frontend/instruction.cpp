#include "instruction.hpp"
#include "../runtime/syscall.hpp"
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

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
    return std::format("mov eax, [rdi+4*{}];"
                       "mov ecx, [rdi+4*{}];"
                       "cmp eax, edx;"
                       "jz addr_{};",
                       this->rs, this->rt, target);
  } else {
    // SLTI
    int32_t imm = this->imm16;
    return std::format("mov eax, [rdi+4*{}];"
                       "cmp eax, {};"
                       "setl al;"
                       "movzx eax, al;"
                       "mov [rdi+4*{}], eax;",
                       this->rs, imm, this->rt);
  }
}

std::string Insn_B::transpile(size_t PC) const {
  if (this->opcode == 0b100010) {
    // USAT: clamp to n bits
    return "int3";
  } else if (this->opcode == 0b111001) {
    // STP
    return std::format("mov eax, [rdi+4*{}];" // base
                       "mov edx, [rdi+4*{}];" // rt1
                       "mov [rax+rdi+4*32+{}], edx;"
                       "mov edx, [rdi+4*{}];" // rt2
                       "mov [rax+rdi+4*32+{}], edx;",
                       this->rd, this->rs, this->imm11, this->imm5,
                       this->imm11 + 4);
  } else {
    // RORI
    return std::format("mov eax, [rdi+4*{}];"
                       "ror eax, {};"
                       "mov [rdi+4*{}], eax;",
                       this->rs, this->imm5, this->rd);
  }
}

std::string Insn_C::transpile(size_t PC) const {
  if (this->opcode == 0b100011) {
    // LD
    return std::format("mov eax, [rdi+4*{}];"
                       "mov rax, [rax+rdi+4*32+{}];"
                       "mov [rdi+4*{}], eax;",
                       this->base, this->offset, this->rt);
  } else {
    // ST
    return std::format("mov eax, [rdi+4*{}];"
                       "mov edx, [rdi+4*{}];"
                       "mov [rax+rdi+4*32+{}], edx;",
                       this->base, this->rt, this->offset);
  }
}

std::string Insn_D::transpile(size_t PC) const {
  if (this->func == 0b000100) {
    // MOVZ
    return std::format("mov eax, [rdi+4*{}];"
                       "mov edx, [rdi+4*{}];"
                       "test eax, eax;"
                       "cmovz edx, [rdi+4*{}];"
                       "mov [rdi+4*{}], edx;",
                       this->rt, this->rd, this->rs, this->rd);
  } else if (this->func == 0b011111) {
    // SUB
    return std::format("mov eax, [rdi+4*{}];"
                       "mov edx, [rdi+4*{}];"
                       "sub eax, edx;"
                       "mov [rdi+4*{}], eax;",
                       this->rs, this->rt, this->rd);
  } else {
    // ADD
    return std::format("mov eax, [rdi+4*{}];"
                       "mov edx, [rdi+4*{}];"
                       "add eax, edx;"
                       "mov [rdi+4*{}], eax;",
                       this->rs, this->rt, this->rd);
  }
}

std::string Insn_SYSCALL::transpile(size_t PC) const {
  return "push rdi;"
         "mov rax, " +
         std::to_string((size_t)syscall_impl) +
         ";"
         "call rax;"
         "pop rdi;";
}
