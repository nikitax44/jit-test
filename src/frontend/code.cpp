#include "code.hpp"
#include <format>

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
  if (start / 4 >= this->insns.size()) {
    throw std::runtime_error(std::format(
        "Code execution out of bounds: start={}, this->insns.size()*4={}",
        start, this->insns.size() * 4));
  }

  size_t ip = start / 4;

  for (; ip != this->insns.size() - 1 && !insns[ip].is_branch(); ++ip) {
    // do nothing, go on
  }

  std::unique_ptr<Stripe> stripe =
      std::make_unique<Stripe>(insns, start, ip * 4);
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
