#include <optional>
struct Insn {

  bool is_branch() const;
  bool branch_can_fallthrough() const; // `call` should return true
  std::optional<size_t> jump_dest() const;
};
