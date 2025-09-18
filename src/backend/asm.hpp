#pragma once
#include "code.hpp"
typedef struct ks_struct ks_engine;

class Assembler {
  ks_engine *ks;

public:
  Assembler();

  Code assemble(const char *code);

  ~Assembler();
};
