// #include <ios>
#include "frontend/code.hpp"
#include "frontend/types.hpp"
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
  std::vector<InsnWrap> ops;
  in.seekg(0, std::ios::end);
  size_t size = in.tellg();
  assert(size % sizeof(uint32_t) == 0);
  ops.resize(size / sizeof(uint32_t));
  in.seekg(0, std::ios::beg);
  in.read(reinterpret_cast<char *>(ops.data()), size);

  Code code(ops);

  // auto code = decoder.compile();
  code.run(memory);
}
