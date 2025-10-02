#pragma once
#include "code.hpp"
#include "instruction.hpp"
#include <asmjit/core/jitruntime.h>
#include <bit>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>

struct StripeLoc {
  size_t start;
  size_t end; // points at the branch instruction
};

class StripeDecoder {
  std::map<size_t, size_t> stripes;
  const uint32_t *code;
  size_t size;

  struct Result {
    size_t end_of_the_stripe;
    std::map<size_t, size_t>::const_iterator following_stripe;
    bool is_inside;
  };

  // O(log n) on emplace
  void add_stripe(StripeLoc stripe, Result res) { // reuse existing Result
    // Result res = this->get(stripe.start);
    if (res.is_inside)
      return;
    if (res.following_stripe != stripes.end() &&
        stripe.end >= res.following_stripe->first) {
      // yay, we found a new one!
      assert(res.following_stripe->second == stripe.end);
      // we have to delete the existing smaller stripe
      stripes.erase(res.following_stripe);
    }

    stripes.emplace(stripe.start, stripe.end);
  }

  InsnWrap at(size_t ip) { return std::bit_cast<InsnWrap>(this->code[ip / 4]); }

public:
  StripeDecoder(const uint32_t *code, size_t count)
      : code(code), size(count * 4) {}

  // O(log n)
  Result get(size_t pos) const {
    if (stripes.empty()) {
      return {0, stripes.end(), false};
    }
    auto it = std::prev(stripes.upper_bound(pos)); // less-than or equal to pos
    if (pos <= it->second) {
      return {it->second, stripes.end(), true};
    }
    ++it;
    if (it == stripes.end()) {
      // all stripes end before us
      return {0, stripes.end(), false};
    }
    return {0, it, false};
  }

  // TODO: remove triple-decoding of the instruction
  void traverse(size_t start) {
    // go through the code starting from the
    // the problem is: the branch might always/never be taken and then we will
    // decode both cases.
    //  one of them might be garbage
    // assume that this case does not occur

    // algorithm: go through stripe until the branch. if
    if (start > this->size)
      return;

    auto res = this->get(start);
    if (res.is_inside)
      return;
    StripeLoc next = res.following_stripe != stripes.end()
                         ? StripeLoc{res.following_stripe->first,
                                     res.following_stripe->second}
                         : StripeLoc{size, size - 4};
    size_t ip = start;
    for (; ip != next.start && !this->at(ip).is_branch(); ip += 4) {
      // do nothing, go on
    }

    if (ip == next.start) {
      // updated the existing stripe (or found unterminated code at the end)
      this->add_stripe({start, next.end}, res);
      return;
    }

    // found a new branch
    this->add_stripe({start, ip}, res);

    if (this->at(ip).branch_can_fallthrough()) {
      traverse(ip + 4);
    }

    std::optional<size_t> jump_dest = this->at(ip).jump_dest(ip);
    if (jump_dest.has_value()) {
      traverse(jump_dest.value());
    } else {
      // cannot determine where jump destination is
    }
  }

  void traverse() {
    // entry point is at the start
    this->traverse(0);
  }

  Code compile() {
    this->traverse();
    Code out;
    std::shared_ptr<asmjit::JitRuntime> rt =
        std::make_shared<asmjit::JitRuntime>();
    for (const auto &stripe : this->stripes) {
      std::unique_ptr<Stripe> s =
          std::make_unique<Stripe>(this->code, stripe.first, stripe.second);
      out.insertStripe(std::move(s));
    }
    return out;
  }
};
