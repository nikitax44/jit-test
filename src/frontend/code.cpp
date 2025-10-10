#include "code.hpp"
#include "types.hpp"
#include <format>
#include <memory>

void Code::run(Cpu &cpu, Memory &mem) {
  std::optional<std::shared_ptr<Stripe>> prev;
  auto stripe = this->get_or_insert(cpu.pc);
  while (true) {
    // update prediction
    if (prev.has_value() && stripe != prev.value()) { // pointer equality
      prev.value()->next = stripe;
    }
    prev = stripe;

    // run
    stripe->invoke(cpu, mem);

    // cpu.pc is updated
    if (!stripe->next.has_value()) {
      // no prediction
      stripe = this->get_or_insert(cpu.pc);
      continue;
    }

    auto next = stripe->next.value().lock();

    if (!next || !next->contains(cpu.pc)) {
      // branch mispredict or predicted stripe expired
      stripe = this->get_or_insert(cpu.pc);
      continue;
    }
    stripe = next;
  }
}

void Code::run_interpret(Cpu &cpu, Memory &mem) {
  while (true) {
    auto res = this->insns[cpu.pc / sizeof(InsnWrap)].interpret(cpu, mem);
    if (res.has_value()) {
      cpu.pc = res.value();
    } else {
      cpu.pc += sizeof(InsnWrap);
    }
  }
}

std::shared_ptr<Stripe> Code::decode(Addr start) const {
  if (start >= this->insns.size() * sizeof(InsnWrap)) {
    throw std::runtime_error(
        std::format("Code execution out of bounds: start={}, "
                    "this->insns.size()*sizeof(InsnWrap)={}",
                    start, this->insns.size() * sizeof(InsnWrap)));
  }

  size_t ip = start / sizeof(InsnWrap);

  for (; ip != this->insns.size() - 1 && !insns[ip].is_branch(); ++ip) {
    // do nothing, go on
  }

  std::shared_ptr<Stripe> stripe =
      std::make_shared<Stripe>(insns, start, ip * sizeof(InsnWrap));
  return stripe;
}

std::optional<std::shared_ptr<Stripe>> Code::get(Addr pc) const {
  auto it = code.lower_bound(pc);
  if (it == code.end())
    return {};
  const std::shared_ptr<Stripe> &stripe = it->second;
  if (pc >= stripe->startPC()) {
    return {stripe};
  } else {
    return {};
  }
}
