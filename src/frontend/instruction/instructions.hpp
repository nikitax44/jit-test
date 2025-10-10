#include "../types.hpp"

struct Insn_A {
  unsigned _opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : _opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  inline Addr jump_dest(Addr pc) const { return pc + (((int32_t)imm16) << 2); }
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
};

struct Insn_C {
  unsigned _opcode : 6;
  unsigned base : 5;
  unsigned rt : 5;
  signed offset : 16;
  Insn_C(uint32_t bits)
      : _opcode(bits >> 26), base(bits >> 21), rt(bits >> 16), offset(bits) {}
  inline Opcode opcode() const { return Opcode(this->_opcode); }
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
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
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
};

struct Insn_SYSCALL {
  unsigned _opcode : 6;
  unsigned code : 20;
  unsigned _func : 6;
  Insn_SYSCALL(uint32_t bits)
      : _opcode(bits >> 26), code(bits >> 6), _func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
};

struct Insn_J {
  unsigned _opcode : 6;
  unsigned index : 26;
  Insn_J(uint32_t bits) : _opcode(bits >> 26), index(bits) {}

  inline Addr jump_dest(Addr pc) const {
    Addr base = (pc >> 28) << 28;
    return base + (((Addr)index) << 2);
  }
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
};

struct Insn_ILLEGAL {
  unsigned bits : 32;
  Insn_ILLEGAL(uint32_t bits) : bits(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr pc) const;
};
