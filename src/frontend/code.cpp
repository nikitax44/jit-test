#include "code.hpp"
#include <format>
#include <memory>

void Code::run(Cpu &cpu, Memory &mem) {
  std::optional<std::shared_ptr<Stripe>> prev;
  auto stripe = this->get(cpu.pc);
  while (true) {
    if (!stripe.has_value()) {
      stripe = this->decode(cpu.pc);
      this->insertStripe(stripe.value());
    }

    std::shared_ptr<Stripe> s = stripe.value();
    if (prev.has_value() && s != prev.value()) {
      prev.value()->next = s;
    }
    prev = s;

    cpu.pc = s->invoke(cpu, mem);
    if (!s->next.has_value()) {
      // no prediction
      stripe = this->get(cpu.pc);
      continue;
    }

    auto next = s->next.value().lock();

    if (!next || !next->contains(cpu.pc)) {
      // branch mispredict or predicted stripe expired
      stripe = this->get(cpu.pc);
      continue;
    }
    stripe = next;
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
