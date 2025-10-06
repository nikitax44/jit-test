#include "../runtime/syscall.hpp"
#include "types.hpp"
#include <asmjit/core/globals.h>
#include <asmjit/x86/x86assembler.h>
#include <asmjit/x86/x86operand.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#define VAR(var) /* size=4 */                                                  \
  asmjit::x86::Mem(asmjit::x86::rdi, sizeof(Reg) * (var), 2)
#define MEM(base, offset) /* scale=1 */                                        \
  asmjit::x86::Mem(asmjit::x86::rsi, (base), 0, sizeof(Reg) * regN + (offset))
#define PC VAR(32)

static_assert(offsetof(Cpu, pc) == 32 * sizeof(Reg),
              "offsetof(CPU, pc) is invalid");

struct Insn_A {
  unsigned _opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : _opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  inline Addr jump_dest(Addr pc) const { return pc + (((int32_t)imm16) << 2); }
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    if (this->opcode() == Opcode::BEQ) {
      Addr target = this->jump_dest(pc);
      a.mov(asmjit::x86::eax, VAR(this->rs));
      a.mov(asmjit::x86::edx, VAR(this->rt));
      a.cmp(asmjit::x86::eax, asmjit::x86::edx);

      auto didJump = a.newLabel();
      a.jz(didJump);
      a.mov(PC, pc + sizeof(InsnWrap));
      a.ret();

      a.bind(didJump);
      a.mov(PC, target);
      a.ret();
    } else if (this->opcode() == Opcode::SLTI) {
      int32_t imm = this->imm16;
      a.mov(asmjit::x86::eax, VAR(this->rs));
      a.cmp(asmjit::x86::eax, imm);
      a.setl(asmjit::x86::al);                    //  al=(uint8_t)(X[rs]<imm)
      a.movzx(asmjit::x86::eax, asmjit::x86::al); // eax=(uint32_t)al
      a.mov(VAR(this->rt), asmjit::x86::eax);
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    if (this->opcode() == Opcode::USAT) {
      // clamp value to n bits
      a.int3();
    } else if (this->opcode() == Opcode::STP) {
      a.mov(asmjit::x86::eax, VAR(this->rd));

      a.mov(asmjit::x86::edx, VAR(this->rs)); // rt1
      a.mov(MEM(asmjit::x86::eax, this->imm11), asmjit::x86::edx);

      a.mov(asmjit::x86::edx, VAR(this->imm5)); // rt2
      a.mov(MEM(asmjit::x86::eax, this->imm11 + sizeof(Reg)), asmjit::x86::edx);
    } else if (this->opcode() == Opcode::RORI) {
      // RORI
      a.mov(asmjit::x86::eax, VAR(this->rs));
      a.ror(asmjit::x86::eax, this->imm5);
      a.mov(VAR(this->rd), asmjit::x86::eax);
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    if (this->opcode() == Opcode::LD) {
      a.mov(asmjit::x86::eax, VAR(this->base));
      a.mov(asmjit::x86::eax, MEM(asmjit::x86::eax, this->offset));
      a.mov(VAR(this->rt), asmjit::x86::eax);
    } else if (this->opcode() == Opcode::ST) {
      a.mov(asmjit::x86::eax, VAR(this->base));
      a.mov(asmjit::x86::edx, VAR(this->rt));
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    if (this->opcode() != Opcode::FUNC) {
      throw std::logic_error("Insn_D's opcode is not FUNC");
    }

    if (this->func() == OpcodeFunc::MOVZ) {
      // if X[rt]==0 { X[rd]=X[rs]; } else { X[rd]=X[rd] }
      a.mov(asmjit::x86::eax, VAR(this->rt));
      a.mov(asmjit::x86::edx, VAR(this->rd));
      a.test(asmjit::x86::eax, asmjit::x86::eax);
      a.cmovz(asmjit::x86::edx, VAR(this->rs));
      a.mov(VAR(this->rd), asmjit::x86::edx);
    } else if (this->func() == OpcodeFunc::SUB) {
      // SUB
      a.mov(asmjit::x86::eax, VAR(this->rs));
      a.mov(asmjit::x86::edx, VAR(this->rt));
      a.sub(asmjit::x86::eax, asmjit::x86::edx);
      a.mov(VAR(this->rd), asmjit::x86::eax);
    } else if (this->func() == OpcodeFunc::ADD) {
      // ADD
      a.mov(asmjit::x86::eax, VAR(this->rs));
      a.mov(asmjit::x86::edx, VAR(this->rt));
      a.add(asmjit::x86::eax, asmjit::x86::edx);
      a.mov(VAR(this->rd), asmjit::x86::eax);
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const { a.int3(); }
};

struct Insn_SYSCALL {
  unsigned _opcode : 6;
  unsigned code : 20;
  unsigned _func : 6;
  Insn_SYSCALL(uint32_t bits)
      : _opcode(bits >> 26), code(bits >> 6), _func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    // rsp%16 == 8 because last thing pushed was return address
    a.push(asmjit::x86::rdi);
    a.push(asmjit::x86::rsi);
    a.sub(asmjit::x86::rsp, 8);
    // rsp%16 == 0
    a.mov(PC, pc); // cpu.pc = pc
    a.mov(asmjit::x86::rax, (size_t)syscall_impl);
    a.call(asmjit::x86::rax);

    a.add(asmjit::x86::rsp, 8);
    a.pop(asmjit::x86::rsi);
    a.pop(asmjit::x86::rdi);

    auto end = a.newLabel();

    // rax contains the payload:tag
    a.sub(asmjit::x86::rax, 1);
    a.jl(end); // if tag==TAG_NOP: do nothing

    a.mov(asmjit::x86::rdx, asmjit::x86::rax);
    a.shr(asmjit::x86::rdx, 32);               // rdx is the payload
    a.mov(asmjit::x86::eax, asmjit::x86::eax); // rax is the tag
    a.mov(asmjit::x86::rcx, (size_t)TABLE);
    // tailcall TABLE[tag-1](rdi, rsi, payload)
    a.jmp(asmjit::x86::Mem(asmjit::x86::rcx, asmjit::x86::rax, 3, 0));

    a.bind(end);
  }
};

struct Insn_J {
  unsigned _opcode : 6;
  unsigned index : 26;
  Insn_J(uint32_t bits) : _opcode(bits >> 26), index(bits) {}

  inline Addr jump_dest(Addr pc) const {
    Addr base = (pc >> 28) << 28;
    return base + (((Addr)index) << 2);
  }
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
    Addr dest = this->jump_dest(pc);

    a.mov(PC, dest);
    a.ret();
  }
};

struct Insn_ILLEGAL {
  unsigned bits : 32;
  Insn_ILLEGAL(uint32_t bits) : bits(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr pc) const {
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

std::optional<Addr> InsnWrap::jump_dest(Addr pc) const {
  switch (opcode()) {
  case Opcode::BEQ:
    return Insn_A(bits).jump_dest(pc);
  case Opcode::J:
    return Insn_J(bits).jump_dest(pc);

  default:
    return {};
  }
}
std::optional<Addr> InsnWrap::const_jump(Addr pc) const {
  if (opcode() == Opcode::J) {
    return Insn_J(bits).jump_dest(pc);
  }
  return {};
}
