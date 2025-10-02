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
  std::map<Addr, std::unique_ptr<Stripe>> code;
  std::span<InsnWrap> insns;

  std::optional<std::reference_wrapper<const Stripe>> get(Addr PC) const;
  std::unique_ptr<Stripe> decode(Addr start) const;

  void insertStripe(std::unique_ptr<Stripe> stripe) {
    Addr endPC = stripe->endPC();
    this->code.emplace(endPC, std::move(stripe));
  }

public:
  Code(std::span<InsnWrap> insns) : code(), insns(insns) {
    if (insns.size() > std::numeric_limits<Addr>::max()) {
      throw std::runtime_error("Program does not fit in the memory");
    }
  }

  void run(void *mem);
};
