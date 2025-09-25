#pragma once
#include <cstdint>
#include <format>
#include <memory>
#include <optional>

struct Insn {
  virtual bool is_branch() const { return false; };
  virtual bool branch_can_fallthrough() const {
    return false;
  }; // `call` should return true
  virtual std::optional<size_t> jump_dest(size_t PC) const { return {}; };
  virtual std::string transpile(size_t PC) const = 0;

  static std::unique_ptr<Insn> decode_insn(uint32_t insn);
};

struct Insn_A : Insn {
  unsigned opcode : 6;
  unsigned rs : 5;
  unsigned rt : 5;
  signed imm16 : 16;
  Insn_A(uint32_t bits)
      : opcode(bits >> 26), rs(bits >> 21), rt(bits >> 16), imm16(bits) {}
  bool is_branch() const override {
    return opcode == 0b010011; // BEQ
  }
  bool branch_can_fallthrough() const override { return true; }
  std::optional<size_t> jump_dest(size_t PC) const override {
    return {PC + (((int32_t)imm16) << 2)};
  }
  std::string transpile(size_t PC) const override;
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
  std::string transpile(size_t PC) const override;
};

struct Insn_C : Insn {
  unsigned opcode : 6;
  unsigned base : 5;
  unsigned rt : 5;
  signed offset : 16;
  Insn_C(uint32_t bits)
      : opcode(bits >> 26), base(bits >> 21), rt(bits >> 16), offset(bits) {}
  std::string transpile(size_t PC) const override;
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
  std::string transpile(size_t PC) const override { return "int3;"; }
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
  std::string transpile(size_t PC) const override { return "int3;"; }
};

struct Insn_SYSCALL : Insn {
  unsigned opcode : 6 = 0;
  unsigned code : 20;
  unsigned func : 6 = 0b011001;
  Insn_SYSCALL(uint32_t bits)
      : opcode(bits >> 26), code(bits >> 6), func(bits) {}
  std::string transpile(size_t PC) const override { return "int3;"; }
};

struct Insn_J : Insn {
  unsigned opcode : 6 = 0b010110;
  unsigned index : 26;
  Insn_J(uint32_t bits) : opcode(bits >> 26), index(bits) {}

  bool is_branch() const override { return true; }
  bool branch_can_fallthrough() const override { return false; }
  std::optional<size_t> jump_dest(size_t PC) const override {
    size_t base = (PC >> 28) << 28;
    return {base + (((size_t)index) << 2)};
  }
  std::string transpile(size_t PC) const override {
    return std::format("jmp addr_{};", this->jump_dest(PC).value());
  }
};

struct Insn_ILLEGAL : Insn {
  unsigned bits : 32;
  Insn_ILLEGAL(uint32_t bits) : bits(bits) {}
  std::string transpile(size_t PC) const override { return "ud2;"; }
};
