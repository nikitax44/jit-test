#pragma once
#include "stripe.hpp"
#include "types.hpp"
#include <limits>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>

class Code {
  // maps from stripe's end to the Stripe
  std::map<Addr, std::shared_ptr<Stripe>> code;
  std::span<InsnWrap> insns;

  std::optional<std::shared_ptr<Stripe>> get(Addr pc) const;
  std::shared_ptr<Stripe> decode(Addr start) const;

  void insertStripe(std::shared_ptr<Stripe> stripe) {
    Addr endPC = stripe->endPC();
    this->code.emplace(endPC, std::move(stripe));
  }

  std::shared_ptr<Stripe> get_or_insert(Addr pc) {
    auto stripe = this->get(pc);
    if (stripe.has_value()) {
      return stripe.value();
    } else {
      auto stripe = this->decode(pc);
      this->insertStripe(stripe);
      return stripe;
    }
  }

public:
  Code(std::span<InsnWrap> insns) : code(), insns(insns) {
    if (insns.size() > std::numeric_limits<Addr>::max()) {
      throw std::runtime_error("Program does not fit in the memory");
    }
  }

  [[noreturn]] void run(Cpu &cpu, Memory &mem);
};
