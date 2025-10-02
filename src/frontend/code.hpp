#pragma once

#include "stripe.hpp"
#include <memory>
#include <stdexcept>
class Code {
  std::map<size_t, std::unique_ptr<Stripe>> code;

  const Stripe &get(size_t PC) const {
    if (code.empty()) {
      throw std::runtime_error("Cannot get stripe from empty program");
    }
    const std::unique_ptr<Stripe> &stripe =
        std::prev(code.upper_bound(PC))->second;
    return *stripe; // less-than or equal to pos
  }

public:
  Code() : code() {}
  void insertStripe(std::unique_ptr<Stripe> stripe) {
    size_t startPC = stripe->startPC();
    this->code.emplace(startPC, std::move(stripe));
  }

  void run(void *mem) const {
    size_t PC = 0;
    while (true) {
      const auto &stripe = this->get(PC);
      PC = stripe.invoke(PC, mem);
    }
  }
};
