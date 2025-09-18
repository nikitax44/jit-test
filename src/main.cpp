#include "backend/asm.hpp"
#include <ios>
#include <iostream>
int main() {
  Assembler assm;
  auto code = assm.assemble(
      // start
      "mov rax, [rsp];"
      "ret;"
      // end
  );
  std::cout << std::hex << std::showbase << code.invoke() << std::endl;
  ;
  std::cout << "Hello!" << std::endl;
}
