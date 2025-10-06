#pragma once
#include <asmjit/x86/x86assembler.h>
#include <cstdint>
#include <optional>
typedef uint32_t Addr;
typedef uint32_t Reg;
inline constexpr size_t regN = 1 << 5;

struct Memory {
  Reg reg[regN];
  uint8_t memory[];
};

enum class Opcode {
  BEQ = 0b010011,
  SLTI = 0b111011,
  USAT = 0b100010,
  STP = 0b111001,
  RORI = 0b001100,
  LD = 0b100011,
  ST = 0b100101,
  J = 0b010110,
  FUNC = 0,
};

enum class OpcodeFunc {
  MOVZ = 0b000100,
  SUB = 0b011111,
  ADD = 0b011010,
  SELC = 0b000011,
  RBIT = 0b011101,
  SYSCALL = 0b011001,
};

struct InsnWrap {
  uint32_t bits;
  inline bool is_branch() const {
    return this->opcode() == Opcode::BEQ || this->opcode() == Opcode::J;
  }
  inline bool branch_can_fallthrough() const {
    return this->opcode() == Opcode::BEQ;
  }
  std::optional<Addr> const_jump(Addr PC) const;
  std::optional<Addr> jump_dest(Addr PC) const;
  void transpile(asmjit::x86::Assembler &a, Addr PC) const;

  inline Opcode opcode() const { return Opcode(bits >> 26); }
};

static_assert(std::is_trivially_copyable_v<InsnWrap> &&
                  sizeof(InsnWrap) == sizeof(uint32_t),
              "InsnWrap should be bitcast'able from uint32_t");
