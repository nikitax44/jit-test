#include "../runtime/syscall.hpp"
#include "types.hpp"
#include <asmjit/x86/x86assembler.h>
#include <asmjit/x86/x86operand.h>
#include <cstdint>
#include <cstdlib>

#define VAR(var) asmjit::x86::Mem(asmjit::x86::rdi, 4 * (var))
#define LD(target, var) a.mov((target), VAR(var))
#define ST(source, var) a.mov(VAR(var), (source))
#define MEM(base, offset)                                                      \
  asmjit::x86::Mem(asmjit::x86::rdi, (base), 1, 4 * 32 + (offset))

struct Insn_A {
  unsigned opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  inline Addr jump_dest(Addr PC) const { return PC + (((int32_t)imm16) << 2); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode == 0b010011) {
      // BEQ
      Addr target = this->jump_dest(PC);
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
};

struct Insn_B {
  unsigned opcode : 6;
  unsigned rd : 5;   // base
  unsigned rs : 5;   // rt1
  unsigned imm5 : 5; // rt2
  signed imm11 : 11;
  Insn_B(uint32_t bits)
      : opcode(bits >> 26), rd(bits >> 21), rs(bits >> 16), imm5(bits >> 11),
        imm11(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode == 0b100010) {
      // USAT: clamp to n bits
      a.int3();
    } else if (this->opcode == 0b111001) {
      // STP
      LD(asmjit::x86::eax, this->rd);

      LD(asmjit::x86::edx, this->rs);
      a.mov(MEM(asmjit::x86::eax, this->imm11), asmjit::x86::edx);

      LD(asmjit::x86::edx, this->imm5);
      a.mov(MEM(asmjit::x86::eax, this->imm11 + 4), asmjit::x86::edx);
    } else {
      // RORI
      LD(asmjit::x86::eax, this->rs);
      a.ror(asmjit::x86::eax, this->imm5);
      ST(asmjit::x86::eax, this->rd);
    }
  }
};

struct Insn_C {
  unsigned opcode : 6;
  unsigned base : 5;
  unsigned rt : 5;
  signed offset : 16;
  Insn_C(uint32_t bits)
      : opcode(bits >> 26), base(bits >> 21), rt(bits >> 16), offset(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode == 0b100011) {
      // LD
      LD(asmjit::x86::eax, this->base);
      a.mov(asmjit::x86::eax, MEM(asmjit::x86::eax, this->offset));
      ST(asmjit::x86::eax, this->rt);
    } else {
      // ST
      LD(asmjit::x86::eax, this->base);
      LD(asmjit::x86::edx, this->rt);
      a.mov(MEM(asmjit::x86::eax, this->offset), asmjit::x86::edx);
    }
  }
};

struct Insn_D {
  unsigned opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  unsigned rd : 5;
  unsigned zero : 5;
  unsigned func : 6;
  Insn_D(uint32_t bits)
      : opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), rd(bits >> 11),
        zero(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
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
};

struct Insn_E {
  unsigned opcode : 6;
  unsigned rd : 5;
  unsigned rs1 : 5;
  unsigned rs2 : 5;
  unsigned zero : 5;
  unsigned func : 6;
  Insn_E(uint32_t bits)
      : opcode(bits >> 26), rd(bits >> 21), rs1(bits >> 16), rs2(bits >> 11),
        zero(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const { a.int3(); }
};

struct Insn_SYSCALL {
  unsigned opcode : 6 = 0;
  unsigned code : 20;
  unsigned func : 6 = 0b011001;
  Insn_SYSCALL(uint32_t bits)
      : opcode(bits >> 26), code(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    a.push(asmjit::x86::rdi);
    a.mov(asmjit::x86::rax, (size_t)syscall_impl);
    a.call(asmjit::x86::rax);
    a.pop(asmjit::x86::rdi);
  }
};

struct Insn_J {
  unsigned opcode : 6 = 0b010110;
  unsigned index : 26;
  Insn_J(uint32_t bits) : opcode(bits >> 26), index(bits) {}

  inline Addr jump_dest(Addr PC) const {
    Addr base = (PC >> 28) << 28;
    return base + (((Addr)index) << 2);
  }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    Addr dest = this->jump_dest(PC);
    a.mov(asmjit::x86::eax, dest);
    a.ret();
  }
};

struct Insn_ILLEGAL {
  unsigned bits : 32;
  Insn_ILLEGAL(uint32_t bits) : bits(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (bits == 1) {
      a.int3();
    } else {
      a.ud2();
    }
  }
};

static void transpile_func(InsnWrap insn, asmjit::x86::Assembler &a, Addr PC) {
  switch (insn.bits & 0b111111) {
  case 0b000100: // MOVZ
  case 0b011111: // SUB
  case 0b011010: // ADD
    return Insn_D(insn.bits).transpile(a, PC);

  case 0b000011: // SELC
  case 0b011101: // RBIT
    return Insn_E(insn.bits).transpile(a, PC);
  case 0b011001: // SYSCALL
    return Insn_SYSCALL(insn.bits).transpile(a, PC);
  default:
    return Insn_ILLEGAL(insn.bits).transpile(a, PC);
  }
}

static void transpile_insn(InsnWrap insn, asmjit::x86::Assembler &a, Addr PC) {
  switch (insn.opcode()) {
  case 0:
    return transpile_func(insn, a, PC);

  case 0b010011: // BEQ
  case 0b111011: // SLTI
    return Insn_A(insn.bits).transpile(a, PC);

  case 0b100010: // USAT
  case 0b111001: // STP
  case 0b001100: // RORI
    return Insn_B(insn.bits).transpile(a, PC);

  case 0b100011: // LD
  case 0b100101: // ST
    return Insn_C(insn.bits).transpile(a, PC);

  case 0b010110: // J
    return Insn_J(insn.bits).transpile(a, PC);

  default:
    return Insn_ILLEGAL(insn.bits).transpile(a, PC);
  }
}

void InsnWrap::transpile(asmjit::x86::Assembler &a, Addr PC) const {
  return transpile_insn(*this, a, PC);
}

std::optional<Addr> InsnWrap::jump_dest(Addr PC) const {
  switch (opcode()) {
  case 0b010011: // BEQ
    return Insn_A(bits).jump_dest(PC);
  case 0b010110: // J
    return Insn_J(bits).jump_dest(PC);

  default:
    return {};
  }
}
