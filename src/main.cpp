#include "backend/asm.hpp"
// #include <ios>
#include "frontend/decoder.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

static char memory[4096];

int main(int argc, char **) {

  // auto code = assm.assemble(
  //     // start
  //     "mov rax, [rsp];"
  //     "ret;"
  //     // end
  // );
  // std::cout << std::hex << std::showbase << code.invoke() << std::endl;
  // ;
  // std::cout << "Hello!" << std::endl;
  std::ifstream in("/tmp/code.bin");
  std::vector<uint32_t> ops;
  in.seekg(0, std::ios::end);
  size_t size = in.tellg();
  assert(size % sizeof(uint32_t) == 0);
  ops.resize(size / sizeof(uint32_t));
  in.seekg(0, std::ios::beg);
  in.read(reinterpret_cast<char *>(ops.data()), size);

  StripeDecoder decoder(ops.data(), ops.size());

  auto code = decoder.compile();

  std::replace(code.begin(), code.end(), ';', '\n');

  if (argc > 1) {
    std::cout << code << std::endl;
    std::cout << "\n// nstripes: " << decoder.stripes.size() << std::endl;
  }

  Assembler assm;
  auto exec = assm.assemble(code.c_str());
  // std::cout << exec.ptr() << std::endl;
  exec.invoke(memory);
}
