#include "../runtime/syscall.hpp"
#include "platform.hpp"
#include "types.hpp"
#include <asmjit/x86/x86assembler.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#if defined(ABI_FASTCALL64)
#  define REG_CPU asmjit::x86::rcx
#  define REG_MEM asmjit::x86::rdx
#  define REG_STACK asmjit::x86::rsp

#  define REG_ZRET asmjit::x86::rax
#  define REG_ERET asmjit::x86::eax
#  define REG_LRET asmjit::x86::al

#  define REG_EBUF asmjit::x86::r9d
#elif defined(ABI_SYSV)
#  define REG_CPU asmjit::x86::rdi
#  define REG_MEM asmjit::x86::rsi
#  define REG_STACK asmjit::x86::rsp

#  define REG_ZRET asmjit::x86::rax
#  define REG_ERET asmjit::x86::eax
#  define REG_LRET asmjit::x86::al

#  define REG_EBUF asmjit::x86::ecx
#elif defined(ABI_CDECL)
#  define REG_CPU asmjit::x86::edi
#  define REG_MEM asmjit::x86::esi
#  define REG_STACK asmjit::x86::esp

#  define REG_ZRET asmjit::x86::eax // 64-bit values are returned as edx:eax
#  define REG_ERET asmjit::x86::eax
#  define REG_LRET asmjit::x86::al

#  define REG_EBUF asmjit::x86::ecx
#endif

#define VAR(var) /* size = 4 */                                                \
  asmjit::x86::Mem(REG_CPU, sizeof(Reg) * (var), 2)
#define MEM(base, offset) /* scale = 1 */                                      \
  asmjit::x86::Mem(REG_MEM, (base), 0, sizeof(Reg) * regN + (offset))
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
      a.mov(REG_ERET, VAR(this->rs));
      a.mov(REG_EBUF, VAR(this->rt));
      a.cmp(REG_ERET, REG_EBUF);

      auto didJump = a.newLabel();
      a.jz(didJump);
      a.mov(PC, pc + sizeof(InsnWrap));
      a.mov(REG_ERET, 0);
      a.ret();

      a.bind(didJump);
      a.mov(PC, target);
      a.mov(REG_ERET, 0);
      a.ret();
    } else if (this->opcode() == Opcode::SLTI) {
      int32_t imm = this->imm16;
      a.mov(REG_ERET, VAR(this->rs));
      a.cmp(REG_ERET, imm);
      a.setl(REG_LRET);            //  al=(uint8_t)(X[rs]<imm)
      a.movzx(REG_ERET, REG_LRET); // eax=(uint32_t)al
      a.mov(VAR(this->rt), REG_ERET);
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
      a.mov(REG_ERET, VAR(this->rd));

      a.mov(REG_EBUF, VAR(this->rs)); // rt1
      a.mov(MEM(REG_ERET, this->imm11), REG_EBUF);

      a.mov(REG_EBUF, VAR(this->imm5)); // rt2
      a.mov(MEM(REG_ERET, this->imm11 + sizeof(Reg)), REG_EBUF);
    } else if (this->opcode() == Opcode::RORI) {
      // RORI
      a.mov(REG_ERET, VAR(this->rs));
      a.ror(REG_ERET, this->imm5);
      a.mov(VAR(this->rd), REG_ERET);
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
      a.mov(REG_ERET, VAR(this->base));
      a.mov(REG_ERET, MEM(REG_ERET, this->offset));
      a.mov(VAR(this->rt), REG_ERET);
    } else if (this->opcode() == Opcode::ST) {
      a.mov(REG_ERET, VAR(this->base));
      a.mov(REG_EBUF, VAR(this->rt));
      a.mov(MEM(REG_ERET, this->offset), REG_EBUF);
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
      a.mov(REG_ERET, VAR(this->rt));
      a.mov(REG_EBUF, VAR(this->rd));
      a.test(REG_ERET, REG_ERET);
      a.cmovz(REG_EBUF, VAR(this->rs));
      a.mov(VAR(this->rd), REG_EBUF);
    } else if (this->func() == OpcodeFunc::SUB) {
      // SUB
      a.mov(REG_ERET, VAR(this->rs));
      a.mov(REG_EBUF, VAR(this->rt));
      a.sub(REG_ERET, REG_EBUF);
      a.mov(VAR(this->rd), REG_ERET);
    } else if (this->func() == OpcodeFunc::ADD) {
      // ADD
      a.mov(REG_ERET, VAR(this->rs));
      a.mov(REG_EBUF, VAR(this->rt));
      a.add(REG_ERET, REG_EBUF);
      a.mov(VAR(this->rd), REG_ERET);
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
    a.mov(PC, pc); // cpu.pc = pc

    a.push(REG_MEM);
    a.push(REG_CPU);
    // sp%16 == 8 on 64-bit systems
#ifdef ABI_FASTCALL64
    a.sub(REG_STACK, 40); // 32 bytes of shadow space
#elif ABI_SYSV
    a.sub(REG_STACK, 8);
#elif ABI_CDECL
    // arguments are already on the stack.
    // function may alter them, so they must be discarded after the call
#endif
    // sp%16 == 0 on 64-bit systems
    a.mov(REG_ZRET, (size_t)syscall_impl);
    a.call(REG_ZRET);

#ifdef ABI_FASTCALL64
    a.add(REG_STACK, 40);
#elif ABI_SYSV
    a.add(REG_STACK, 8);
#endif

#ifdef ABI_CDECL
    // discard the arguments.
    // edi, esi are callee-saved registers.
    a.add(REG_STACK, 4 * 2);
#else
    a.pop(REG_CPU);
    a.pop(REG_MEM);
#endif

    auto end = a.newLabel();

    a.test(REG_ERET, REG_ERET); // ERET is always the lower 32-bits of result.
    a.jz(end);                  // if tag==TAG_NOP: do nothing
    // PC is already set, rax/edx:eax must persist
    a.ret();
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
    a.mov(REG_ERET, 0);
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
