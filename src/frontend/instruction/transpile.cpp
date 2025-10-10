#include "../../runtime/syscall.hpp"
#include "../platform.hpp"
#include "instructions.hpp"
#include <asmjit/x86/x86assembler.h>
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
  asmjit::x86::dword_ptr(REG_CPU, sizeof(Reg) * (var))
#define MEM(base, offset) /* scale = 1 */                                      \
  asmjit::x86::dword_ptr(REG_MEM, (base), 0, sizeof(Reg) * regN + (offset))
#define PC VAR(32)

static_assert(offsetof(Cpu, pc) == 32 * sizeof(Reg),
              "offsetof(CPU, pc) is invalid");

void Insn_A::transpile(asmjit::x86::Assembler &a, Addr pc) const {
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

void Insn_B::transpile(asmjit::x86::Assembler &a, Addr pc) const {
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
    a.mov(REG_ERET, VAR(this->rs));
    a.ror(REG_ERET, this->imm5);
    a.mov(VAR(this->rd), REG_ERET);
  } else {
    throw std::logic_error("Invalid opcode in Insn_B");
  }
}

void Insn_C::transpile(asmjit::x86::Assembler &a, Addr pc) const {
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

void Insn_D::transpile(asmjit::x86::Assembler &a, Addr pc) const {
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
    a.mov(REG_ERET, VAR(this->rs));
    a.mov(REG_EBUF, VAR(this->rt));
    a.sub(REG_ERET, REG_EBUF);
    a.mov(VAR(this->rd), REG_ERET);
  } else if (this->func() == OpcodeFunc::ADD) {
    a.mov(REG_ERET, VAR(this->rs));
    a.mov(REG_EBUF, VAR(this->rt));
    a.add(REG_ERET, REG_EBUF);
    a.mov(VAR(this->rd), REG_ERET);
  } else {
    throw std::logic_error("Invalid func in Insn_D");
  }
}

void Insn_E::transpile(asmjit::x86::Assembler &a, Addr pc) const { a.int3(); }

void Insn_SYSCALL::transpile(asmjit::x86::Assembler &a, Addr pc) const {
  a.mov(PC, pc); // cpu.pc = pc

  // ABI_CDECL: push syscall_impl arguments
  // ABI_SYSV:       preserve caller-saved registers
  // ABI_FASTCALL64: preserve caller-saved registers
  a.push(REG_MEM);
  a.push(REG_CPU);

  // align the stack to 16 bytes(only for 64-bit)
#ifdef ABI_FASTCALL64
  a.sub(REG_STACK, 8 + 32); // alignment + 32 bytes of shadow space
#elif ABI_SYSV
  a.sub(REG_STACK, 8);
#endif

  // arguments are already on the stack.
  // function may alter them, so they must be discarded after the call

  // invoke syscall
  a.mov(REG_ZRET, (size_t)syscall_impl);
  a.call(REG_ZRET);

#ifdef ABI_FASTCALL64
  a.add(REG_STACK, 8 + 32);
#elif ABI_SYSV
  a.add(REG_STACK, 8);
#endif

#ifdef ABI_CDECL
  // discard the arguments.
  // REG_MEM and REG_CPU are callee-saved registers.
  a.add(REG_STACK, 4 * 2);
#else
  // restore registers
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

void Insn_J::transpile(asmjit::x86::Assembler &a, Addr pc) const {
  Addr dest = this->jump_dest(pc);

  a.mov(PC, dest);
  a.mov(REG_ERET, 0);
  a.ret();
}

void Insn_ILLEGAL::transpile(asmjit::x86::Assembler &a, Addr pc) const {
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
