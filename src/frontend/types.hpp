#pragma once
#include <asmjit/x86/x86assembler.h>
#include <cstdint>
#include <optional>
typedef uint32_t Addr;

struct Memory {
  uint32_t reg[32];
  uint8_t memory[];
};

struct InsnWrap {
  uint32_t bits;
  inline bool is_branch() const {
    // BEQ or J
    return this->opcode() == 0b010011 || this->opcode() == 0b010110;
  }
  inline bool branch_can_fallthrough() const {
    // BEQ
    return this->opcode() == 0b010011;
  }
  std::optional<Addr> const_jump(Addr PC) const;
  std::optional<Addr> jump_dest(Addr PC) const;
  void transpile(asmjit::x86::Assembler &a, Addr PC) const;

  inline uint8_t opcode() const { return bits >> 26; }
};

static_assert(std::is_trivially_copyable_v<InsnWrap> &&
                  sizeof(InsnWrap) == sizeof(uint32_t),
              "InsnWrap should be bitcast'able from uint32_t");
