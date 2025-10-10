#include "../../runtime/syscall.hpp"
#include "instructions.hpp"
#include <stdexcept>

std::optional<Addr> Insn_A::interpret(Cpu &cpu, Memory &mem) const {
  if (this->opcode() == Opcode::BEQ) {
    if (cpu.reg[this->rs] == cpu.reg[this->rt]) {
      return this->jump_dest(cpu.pc);
    } else {
      return {};
    }
  } else if (this->opcode() == Opcode::SLTI) {
    cpu.reg[this->rt] = ((int32_t)cpu.reg[this->rs]) < this->imm16;
    return {};
  } else {
    throw std::logic_error("Invalid opcode in Insn_A: ");
  }
}

std::optional<Addr> Insn_B::interpret(Cpu &cpu, Memory &mem) const {
  if (this->opcode() == Opcode::USAT) {
    // clamp value to n bits
    throw std::logic_error("USAT not implemented");
  } else if (this->opcode() == Opcode::STP) {
    mem.write(cpu.reg[this->rd] + this->imm11, cpu.reg[this->rs]);
    mem.write(cpu.reg[this->rd] + this->imm11 + sizeof(Reg),
              cpu.reg[this->imm5]);
    return {};
  } else if (this->opcode() == Opcode::RORI) {
    cpu.reg[this->rd] = std::rotr(cpu.reg[this->rs], this->imm5);
    return {};
  } else {
    throw std::logic_error("Invalid opcode in Insn_B");
  }
}

std::optional<Addr> Insn_C::interpret(Cpu &cpu, Memory &mem) const {
  if (this->opcode() == Opcode::LD) {
    cpu.reg[this->rt] = mem.read(cpu.reg[this->base] + this->offset);
    return {};
  } else if (this->opcode() == Opcode::ST) {
    mem.write(cpu.reg[this->base] + this->offset, cpu.reg[this->rt]);
    return {};
  } else {
    throw std::logic_error("Invalid opcode in Insn_C");
  }
}

std::optional<Addr> Insn_D::interpret(Cpu &cpu, Memory &mem) const {
  if (this->opcode() != Opcode::FUNC) {
    throw std::logic_error("Insn_D's opcode is not FUNC");
  }

  if (this->func() == OpcodeFunc::MOVZ) {
    if (cpu.reg[this->rt] == 0) {
      cpu.reg[this->rd] = cpu.reg[this->rs];
    }
    return {};
  } else if (this->func() == OpcodeFunc::SUB) {
    cpu.reg[this->rd] = cpu.reg[this->rs] - cpu.reg[this->rt];
    return {};
  } else if (this->func() == OpcodeFunc::ADD) {
    cpu.reg[this->rd] = cpu.reg[this->rs] + cpu.reg[this->rt];
    return {};
  } else {
    throw std::logic_error("Invalid func in Insn_D");
  }
}

std::optional<Addr> Insn_E::interpret(Cpu &cpu, Memory &mem) const {
  throw std::logic_error("Insn_E is not implemented");
}

std::optional<Addr> Insn_SYSCALL::interpret(Cpu &cpu, Memory &mem) const {
  uint64_t res = syscall_impl(cpu, mem);
  if ((uint32_t)res != 0) {
    TABLE[(uint32_t)res - 1](cpu, mem, res >> 32);
    return cpu.pc;
  } else {
    return {};
  }
}

std::optional<Addr> Insn_J::interpret(Cpu &cpu, Memory &mem) const {
  return this->jump_dest(cpu.pc);
}

std::optional<Addr> Insn_ILLEGAL::interpret(Cpu &cpu, Memory &mem) const {
  switch (bits) {
  case 1:
    asm volatile("int3");
    break;
  case 2:
    break;
  default:
    throw std::runtime_error("Invalid instruction");
  }
  return {};
}
