#include "../runtime/syscall.hpp"
#include "types.hpp"
#include <asmjit/x86/x86assembler.h>
#include <asmjit/x86/x86operand.h>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#define VAR(var) asmjit::x86::Mem(asmjit::x86::rdi, sizeof(InsnWrap) * (var))
#define LD(target, var) a.mov((target), VAR(var))
#define ST(source, var) a.mov(VAR(var), (source))
#define MEM(base, offset) /* scale=1 */                                        \
  asmjit::x86::Mem(asmjit::x86::rdi, (base), 0, sizeof(Reg) * regN + (offset))

struct Insn_A {
  unsigned _opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : _opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  inline Addr jump_dest(Addr PC) const { return PC + (((int32_t)imm16) << 2); }
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode() == Opcode::BEQ) {
      Addr target = this->jump_dest(PC);
      LD(asmjit::x86::eax, this->rs);
      LD(asmjit::x86::edx, this->rt);
      a.cmp(asmjit::x86::eax, asmjit::x86::edx);

      auto didJump = a.newLabel();
      a.jz(didJump);
      a.mov(asmjit::x86::eax, PC + sizeof(InsnWrap));
      a.ret();

      a.bind(didJump);
      a.mov(asmjit::x86::eax, target);
      a.ret();
    } else if (this->opcode() == Opcode::SLTI) {
      int32_t imm = this->imm16;
      LD(asmjit::x86::eax, this->rs);
      a.cmp(asmjit::x86::eax, imm);
      a.setl(asmjit::x86::al);                    //  al=(uint8_t)(X[rs]<imm)
      a.movzx(asmjit::x86::eax, asmjit::x86::al); // eax=(uint32_t)al
      ST(asmjit::x86::eax, this->rt);
    } else {
      throw std::logic_error("Invalid opcode in Insn_A: ");
    }
  }
};

struct Insn_B {
  unsigned _opcode : 6;
  unsigned rd : 5;   // base
  unsigned rs : 5;   // rt1
  unsigned imm5 : 5; // rt2
  signed imm11 : 11;
  Insn_B(uint32_t bits)
      : _opcode(bits >> 26), rd(bits >> 21), rs(bits >> 16), imm5(bits >> 11),
        imm11(bits) {}
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode() == Opcode::USAT) {
      // clamp value to n bits
      a.int3();
    } else if (this->opcode() == Opcode::STP) {
      LD(asmjit::x86::eax, this->rd);

      LD(asmjit::x86::edx, this->rs); // rt1
      a.mov(MEM(asmjit::x86::eax, this->imm11), asmjit::x86::edx);

      LD(asmjit::x86::edx, this->imm5); // rt2
      a.mov(MEM(asmjit::x86::eax, this->imm11 + sizeof(Reg)), asmjit::x86::edx);
    } else if (this->opcode() == Opcode::RORI) {
      // RORI
      LD(asmjit::x86::eax, this->rs);
      a.ror(asmjit::x86::eax, this->imm5);
      ST(asmjit::x86::eax, this->rd);
    } else {
      throw std::logic_error("Invalid opcode in Insn_B");
    }
  }
};

struct Insn_C {
  unsigned _opcode : 6;
  unsigned base : 5;
  unsigned rt : 5;
  signed offset : 16;
  Insn_C(uint32_t bits)
      : _opcode(bits >> 26), base(bits >> 21), rt(bits >> 16), offset(bits) {}
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode() == Opcode::LD) {
      LD(asmjit::x86::eax, this->base);
      a.mov(asmjit::x86::eax, MEM(asmjit::x86::eax, this->offset));
      ST(asmjit::x86::eax, this->rt);
    } else if (this->opcode() == Opcode::ST) {
      LD(asmjit::x86::eax, this->base);
      LD(asmjit::x86::edx, this->rt);
      a.mov(MEM(asmjit::x86::eax, this->offset), asmjit::x86::edx);
    } else {
      throw std::logic_error("Invalid opcode in Insn_C");
    }
  }
};

struct Insn_D {
  unsigned _opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  unsigned rd : 5;
  unsigned zero : 5;
  unsigned _func : 6;
  Insn_D(uint32_t bits)
      : _opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), rd(bits >> 11),
        zero(bits >> 6), _func(bits) {}
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  inline OpcodeFunc func() const { return OpcodeFunc(this->_func); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    if (this->opcode() != Opcode::FUNC) {
      throw std::logic_error("Insn_D's opcode is not FUNC");
    }

    if (this->func() == OpcodeFunc::MOVZ) {
      // if X[rt]==0 { X[rd]=X[rs]; } else { X[rd]=X[rd] }
      LD(asmjit::x86::eax, this->rt);
      LD(asmjit::x86::edx, this->rd);
      a.test(asmjit::x86::eax, asmjit::x86::eax);
      a.cmovz(asmjit::x86::edx, VAR(this->rs));
      ST(asmjit::x86::edx, this->rd);
    } else if (this->func() == OpcodeFunc::SUB) {
      // SUB
      LD(asmjit::x86::eax, this->rs);
      LD(asmjit::x86::edx, this->rt);
      a.sub(asmjit::x86::eax, asmjit::x86::edx);
      ST(asmjit::x86::eax, this->rd);
    } else if (this->func() == OpcodeFunc::ADD) {
      // ADD
      LD(asmjit::x86::eax, this->rs);
      LD(asmjit::x86::edx, this->rt);
      a.add(asmjit::x86::eax, asmjit::x86::edx);
      ST(asmjit::x86::eax, this->rd);
    } else {
      throw std::logic_error("Invalid func in Insn_D");
    }
  }
};

struct Insn_E {
  unsigned _opcode : 6;
  unsigned rd : 5;
  unsigned rs1 : 5;
  unsigned rs2 : 5;
  unsigned zero : 5;
  unsigned _func : 6;
  Insn_E(uint32_t bits)
      : _opcode(bits >> 26), rd(bits >> 21), rs1(bits >> 16), rs2(bits >> 11),
        zero(bits >> 6), _func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const { a.int3(); }
};

struct Insn_SYSCALL {
  unsigned _opcode : 6;
  unsigned code : 20;
  unsigned _func : 6;
  Insn_SYSCALL(uint32_t bits)
      : _opcode(bits >> 26), code(bits >> 6), _func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    // rsp%16 == 8 because last thing pushed was return address
    a.push(asmjit::x86::rdi);
    // rsp%16 == 0
    a.mov(asmjit::x86::rsi, PC);
    a.mov(asmjit::x86::rax, (size_t)syscall_impl);
    a.call(asmjit::x86::rax);
    a.pop(asmjit::x86::rdi);

    auto end = a.newLabel();

    // rax contains the payload:tag
    a.sub(asmjit::x86::rax, 1);
    a.jl(end); // if tag==TAG_NOP: do nothing

    a.mov(asmjit::x86::rsi, asmjit::x86::rax);
    a.shr(asmjit::x86::rsi, 32);               // rsi is the payload
    a.mov(asmjit::x86::eax, asmjit::x86::eax); // rax is the tag
    a.mov(asmjit::x86::rdx, (size_t)TABLE);
    // tailcall TABLE[tag-1](rdi, payload)
    a.jmp(asmjit::x86::Mem(asmjit::x86::rdx, asmjit::x86::rax, 3, 0));

    a.bind(end);
  }
};

struct Insn_J {
  unsigned _opcode : 6;
  unsigned index : 26;
  Insn_J(uint32_t bits) : _opcode(bits >> 26), index(bits) {}

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
    switch (bits) {
    case 1:
      a.int3();
      break;
    case 2:
      a.nop();
      break;
    default:
      a.ud2();
    }
  }
};

static void transpile_func(InsnWrap insn, asmjit::x86::Assembler &a, Addr PC) {
  switch (OpcodeFunc(insn.bits & 0b111111)) {
  case OpcodeFunc::MOVZ:
  case OpcodeFunc::SUB:
  case OpcodeFunc::ADD:
    return Insn_D(insn.bits).transpile(a, PC);

  case OpcodeFunc::SELC:
  case OpcodeFunc::RBIT:
    return Insn_E(insn.bits).transpile(a, PC);
  case OpcodeFunc::SYSCALL:
    return Insn_SYSCALL(insn.bits).transpile(a, PC);
  default:
    return Insn_ILLEGAL(insn.bits).transpile(a, PC);
  }
}

static void transpile_insn(InsnWrap insn, asmjit::x86::Assembler &a, Addr PC) {
  switch (insn.opcode()) {
  case Opcode::FUNC:
    return transpile_func(insn, a, PC);

  case Opcode::BEQ:
  case Opcode::SLTI:
    return Insn_A(insn.bits).transpile(a, PC);

  case Opcode::USAT:
  case Opcode::STP:
  case Opcode::RORI:
    return Insn_B(insn.bits).transpile(a, PC);

  case Opcode::LD:
  case Opcode::ST:
    return Insn_C(insn.bits).transpile(a, PC);

  case Opcode::J:
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
  case Opcode::BEQ:
    return Insn_A(bits).jump_dest(PC);
  case Opcode::J:
    return Insn_J(bits).jump_dest(PC);

  default:
    return {};
  }
}
std::optional<Addr> InsnWrap::const_jump(Addr PC) const {
  if (opcode() == Opcode::J) {
    return Insn_J(bits).jump_dest(PC);
  }
  return {};
}
