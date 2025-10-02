#pragma once
#include "types.hpp"
#include <asmjit/x86/x86assembler.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

struct Insn {
  virtual void transpile(asmjit::x86::Assembler &a, Addr PC) const = 0;
};

struct Insn_A : Insn {
  unsigned opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  Addr jump_dest(Addr PC) const { return PC + (((int32_t)imm16) << 2); }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override;
};

struct Insn_B : Insn {
  unsigned opcode : 6;
  unsigned rd : 5;   // base
  unsigned rs : 5;   // rt1
  unsigned imm5 : 5; // rt2
  signed imm11 : 11;
  Insn_B(uint32_t bits)
      : opcode(bits >> 26), rd(bits >> 21), rs(bits >> 16), imm5(bits >> 11),
        imm11(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override;
};

struct Insn_C : Insn {
  unsigned opcode : 6;
  unsigned base : 5;
  unsigned rt : 5;
  signed offset : 16;
  Insn_C(uint32_t bits)
      : opcode(bits >> 26), base(bits >> 21), rt(bits >> 16), offset(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override;
};

struct Insn_D : Insn {
  unsigned opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  unsigned rd : 5;
  unsigned zero : 5;
  unsigned func : 6;
  Insn_D(uint32_t bits)
      : opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), rd(bits >> 11),
        zero(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override;
};

struct Insn_E : Insn {
  unsigned opcode : 6;
  unsigned rd : 5;
  unsigned rs1 : 5;
  unsigned rs2 : 5;
  unsigned zero : 5;
  unsigned func : 6;
  Insn_E(uint32_t bits)
      : opcode(bits >> 26), rd(bits >> 21), rs1(bits >> 16), rs2(bits >> 11),
        zero(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override {
    a.int3();
  }
};

struct Insn_SYSCALL : Insn {
  unsigned opcode : 6 = 0;
  unsigned code : 20;
  unsigned func : 6 = 0b011001;
  Insn_SYSCALL(uint32_t bits)
      : opcode(bits >> 26), code(bits >> 6), func(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override;
};

struct Insn_J : Insn {
  unsigned opcode : 6 = 0b010110;
  unsigned index : 26;
  Insn_J(uint32_t bits) : opcode(bits >> 26), index(bits) {}

  Addr jump_dest(Addr PC) const {
    Addr base = (PC >> 28) << 28;
    return base + (((Addr)index) << 2);
  }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override {
    Addr dest = this->jump_dest(PC);
    a.mov(asmjit::x86::eax, dest);
    a.ret();
  }
};

struct Insn_ILLEGAL : Insn {
  unsigned bits : 32;
  Insn_ILLEGAL(uint32_t bits) : bits(bits) {}
  void transpile(asmjit::x86::Assembler &a, Addr PC) const override {
    if (bits == 1) {
      a.int3();
    } else {
      a.ud2();
    }
  }
};

struct InsnWrap {
  uint32_t bits;
  bool is_branch() const {
    // BEQ or J
    return this->opcode() == 0b010011 || this->opcode() == 0b010110;
  }
  bool branch_can_fallthrough() const {
    // BEQ
    return this->opcode() == 0b010011;
  }
  std::optional<Addr> jump_dest(Addr PC) const {
    switch (opcode()) {
    case 0b010011: // BEQ
      return Insn_A(bits).jump_dest(PC);
    case 0b010110: // J
      return Insn_J(bits).jump_dest(PC);

    default:
      return {};
    }
  }
  void transpile(asmjit::x86::Assembler &a, Addr PC) const {
    return decode_insn()->transpile(a, PC);
  }

private:
  inline uint8_t opcode() const { return bits >> 26; }
  std::unique_ptr<Insn> decode_insn() const;
};

static_assert(std::is_trivially_copyable_v<InsnWrap> &&
                  sizeof(InsnWrap) == sizeof(uint32_t),
              "InsnWrap should be bitcast'able from uint32_t");
