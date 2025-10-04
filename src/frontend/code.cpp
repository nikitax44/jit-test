#include "code.hpp"
#include <format>
#include <memory>

void Code::run(Memory &mem) {
  Addr PC = 0;
  std::optional<std::shared_ptr<Stripe>> prev;
  auto stripe = this->get(PC);
  while (true) {
    if (!stripe.has_value()) {
      stripe = this->decode(PC);
      this->insertStripe(stripe.value());
    }

    std::shared_ptr<Stripe> s = stripe.value();
    if (prev.has_value() && s != prev.value()) {
      prev.value()->next = s;
    }
    prev = s;

    PC = s->invoke(PC, mem);
    if (!s->next.has_value()) {
      // no prediction
      stripe = this->get(PC);
      continue;
    }

    auto next = s->next.value().lock();

    if (!next || !next->contains(PC)) {
      // branch mispredict or predicted stripe expired
      stripe = this->get(PC);
      continue;
    }
    stripe = next;
  }
}

std::shared_ptr<Stripe> Code::decode(Addr start) const {
  if (start / 4 >= this->insns.size()) {
    throw std::runtime_error(std::format(
        "Code execution out of bounds: start={}, this->insns.size()*4={}",
        start, this->insns.size() * 4));
  }

  size_t ip = start / 4;

  for (; ip != this->insns.size() - 1 && !insns[ip].is_branch(); ++ip) {
    // do nothing, go on
  }

  std::shared_ptr<Stripe> stripe =
      std::make_shared<Stripe>(insns, start, ip * 4);
  return stripe;
}

std::optional<std::shared_ptr<Stripe>> Code::get(Addr PC) const {
  auto it = code.lower_bound(PC);
  if (it == code.end())
    return {};
  const std::shared_ptr<Stripe> &stripe = it->second;
  if (PC >= stripe->startPC()) {
    return {stripe};
  } else {
    return {};
  }
}
