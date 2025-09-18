#include "instruction.hpp"
#include <cassert>
#include <cstddef>
#include <map>
#include <optional>

struct Stripe {
  size_t start;
  size_t end; // points at the branch instruction
};

class StripeDecoder {
  std::map<size_t, size_t> stripes;
  const Insn *code;
  size_t size;

  struct Result {
    size_t end_of_the_stripe;
    std::map<size_t, size_t>::const_iterator following_stripe;
    bool is_inside;
  };

  // O(log n) on emplace
  void add_stripe(Stripe stripe, Result res) { // reuse existing Result
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

public:
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

  void traverse(size_t start) {
    // go through the code starting from the
    // the problem is: the branch might always/never be taken and then we will
    // decode both cases.
    //  one of them might be garbage
    // assume that this case does not occur

    // algorithm: go through stripe until the branch. if

    auto res = this->get(start);
    if (res.is_inside)
      return;
    Stripe next =
        res.following_stripe != stripes.end()
            ? Stripe{res.following_stripe->first, res.following_stripe->second}
            : Stripe{size, size - 1};
    size_t ip = start;
    for (; ip != next.start && !this->code[ip].is_branch(); ++ip) {
      // do nothing, go on
    }

    if (ip == next.start) {
      // updated the existing stripe (or found unterminated code at the end)
      this->add_stripe({start, next.end}, res);
      return;
    }

    // found a new branch
    this->add_stripe({start, ip}, res);

    if (this->code[ip].branch_can_fallthrough()) {
      traverse(ip + 1);
    }

    std::optional<size_t> jump_dest = this->code[ip].jump_dest();
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
};
