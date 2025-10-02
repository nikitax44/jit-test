#include "code.hpp"
// #include <iostream>

void Code::run(void *mem) {
  Addr PC = 0;
  while (true) {
    auto stripe = this->get(PC);
    if (stripe.has_value()) {
      PC = stripe.value().get().invoke(PC, mem);
    } else {
      // std::cerr << "new stripe with PC = " << PC << std::endl;
      this->insertStripe(this->decode(PC));
      continue; // re-run with the same PC
    }
  }
}

std::unique_ptr<Stripe> Code::decode(Addr start) const {
  if (start > this->insns.size() * 4) {
    throw std::runtime_error("Code execution out of bounds");
  }

  Addr ip = start;

  for (; ip != (this->insns.size() - 1) * 4 && !insns[ip / 4].is_branch();
       ip += 4) {
    // do nothing, go on
  }

  std::unique_ptr<Stripe> stripe =
      std::make_unique<Stripe>(insns.data(), start, ip);
  return stripe;
}

std::optional<std::reference_wrapper<const Stripe>> Code::get(Addr PC) const {
  auto it = code.lower_bound(PC);
  if (it == code.end())
    return {};
  const std::unique_ptr<Stripe> &stripe = it->second;
  if (PC >= stripe->startPC()) {
    return {*stripe};
  } else {
    return {};
  }
}
