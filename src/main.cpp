// #include <ios>
#include "frontend/code.hpp"
#include "frontend/types.hpp"
#include "runtime/exit.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

static bool load_insns(const char *filename, std::vector<InsnWrap> &ops) {
  std::ifstream in(filename, std::ios::binary);
  if (!in) {
    std::cerr << "Error: cannot open file '" << filename << "'\n";
    return false;
  }

  in.seekg(0, std::ios::end);
  if (!in) {
    std::cerr << "Error: seek to end failed for '" << filename << "'\n";
    return false;
  }

  std::streampos endPos = in.tellg();
  if (endPos == std::streampos(-1)) {
    std::cerr << "Error: tellg failed for '" << filename << "'\n";
    return false;
  }

  std::size_t size = static_cast<std::size_t>(endPos);

  if (size % sizeof(InsnWrap) != 0) {
    std::cerr << "Error: file size (" << size << " bytes) is not a multiple of "
              << sizeof(InsnWrap) << " bytes\n";
    return false;
  }

  ops.resize(size / sizeof(InsnWrap));

  in.seekg(0, std::ios::beg);
  if (!in) {
    std::cerr << "Error: seek to beginning failed for '" << filename << "'\n";
    return false;
  }

  in.read(reinterpret_cast<char *>(ops.data()), size);
  if (!in) {
    std::cerr << "Error: read failed (expected " << size << " bytes)\n";
    return false;
  }

  if (in.peek() != std::ifstream::traits_type::eof()) {
    std::cerr << "Error: extra data found after expected end of file\n";
    return false;
  }

  return true;
}

static union {
  Memory memory;
  char bytes[4096];
};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <code file>" << std::endl;
    return 1;
  }
  const char *filename = argv[1];

  std::vector<InsnWrap> ops;

  if (!load_insns(filename, ops)) {
    return -1;
  }

  Code code(ops);

  try {
    Cpu cpu = {0};
    code.run(cpu, memory);
  } catch (exit_exception exc) {
    std::cout << "exit" << std::endl;
    return exc.status();
  }
}
