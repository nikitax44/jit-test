#include "instruction.hpp"
#include "../runtime/syscall.hpp"
#include <asmjit/x86/x86operand.h>
#include <cstdint>
#include <cstdlib>
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

#define VAR(var) asmjit::x86::Mem(asmjit::x86::rdi, 4 * (var))
#define LD(target, var) a.mov((target), VAR(var))
#define ST(source, var) a.mov(VAR(var), (source))

void Insn_A::transpile(asmjit::x86::Assembler &a, size_t PC) const {
  if (this->opcode == 0b010011) {
    // BEQ
    size_t target = this->jump_dest(PC).value();
    LD(asmjit::x86::eax, this->rs);
    LD(asmjit::x86::edx, this->rt);
    a.cmp(asmjit::x86::eax, asmjit::x86::edx);

    auto didJump = a.newLabel();
    a.jz(didJump);
    a.mov(asmjit::x86::eax, PC + 4);
    a.ret();

    a.bind(didJump);
    a.mov(asmjit::x86::eax, target);
    a.ret();
  } else {
    // SLTI
    int32_t imm = this->imm16;
    LD(asmjit::x86::eax, this->rs);
    a.cmp(asmjit::x86::eax, imm);
    a.setl(asmjit::x86::al);
    a.movzx(asmjit::x86::eax, asmjit::x86::al);
    ST(asmjit::x86::eax, this->rt);
  }
}

void Insn_B::transpile(asmjit::x86::Assembler &a, size_t PC) const {
  if (this->opcode == 0b100010) {
    // USAT: clamp to n bits
    a.int3();
  } else if (this->opcode == 0b111001) {
    // STP
    LD(asmjit::x86::eax, this->rd);

    LD(asmjit::x86::edx, this->rs);
    a.mov(asmjit::x86::Mem(asmjit::x86::rax, asmjit::x86::rdi, 1,
                           4 * 32 + this->imm11),
          asmjit::x86::edx);

    LD(asmjit::x86::edx, this->imm5);
    a.mov(asmjit::x86::Mem(asmjit::x86::rax, asmjit::x86::rdi, 1,
                           4 * 32 + this->imm11 + 4),
          asmjit::x86::edx);
  } else {
    // RORI
    LD(asmjit::x86::eax, this->rs);
    a.ror(asmjit::x86::eax, this->imm5);
    ST(asmjit::x86::eax, this->rd);
  }
}

void Insn_C::transpile(asmjit::x86::Assembler &a, size_t PC) const {
  if (this->opcode == 0b100011) {
    // LD
    LD(asmjit::x86::eax, this->base);
    a.mov(asmjit::x86::eax, asmjit::x86::Mem(asmjit::x86::rax, asmjit::x86::rdi,
                                             1, 4 * 32 + this->offset));
    ST(asmjit::x86::eax, this->rt);
  } else {
    // ST
    LD(asmjit::x86::eax, this->base);
    LD(asmjit::x86::edx, this->rt);
    a.mov(asmjit::x86::Mem(asmjit::x86::rax, asmjit::x86::rdi, 1,
                           4 * 32 + this->offset),
          asmjit::x86::edx);
  }
}

void Insn_D::transpile(asmjit::x86::Assembler &a, size_t PC) const {
  if (this->func == 0b000100) {
    // MOVZ
    LD(asmjit::x86::eax, this->rt);
    LD(asmjit::x86::edx, this->rd);
    a.test(asmjit::x86::eax, asmjit::x86::eax);
    a.cmovz(asmjit::x86::edx, VAR(this->rs));
    ST(asmjit::x86::edx, this->rd);
  } else if (this->func == 0b011111) {
    // SUB
    LD(asmjit::x86::eax, this->rt);
    LD(asmjit::x86::edx, this->rd);
    a.sub(asmjit::x86::eax, asmjit::x86::edx);
    ST(asmjit::x86::eax, this->rd);
  } else {
    // ADD
    LD(asmjit::x86::eax, this->rt);
    LD(asmjit::x86::edx, this->rd);
    a.add(asmjit::x86::eax, asmjit::x86::edx);
    ST(asmjit::x86::eax, this->rd);
  }
}

void Insn_SYSCALL::transpile(asmjit::x86::Assembler &a, size_t PC) const {
  a.push(asmjit::x86::rdi);
  a.mov(asmjit::x86::rax, (size_t)syscall_impl);
  a.call(asmjit::x86::rax);
  a.pop(asmjit::x86::rdi);
}
