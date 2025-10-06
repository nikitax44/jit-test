#!/usr/bin/env bash
set -e
cat >build.ninja <<EOF
cflags = $CFLAGS -std=c++20 -O3 -g3
rule cxx
  command = $CXX \$cflags -MMD -MF \$out.d \$in -o \$out -c
  depfile = \$out.d
  deps = gcc
  description = CXX \$out

rule as
  command = $CXX -Wa,--noexecstack -c \$in -o \$out
  description = AS \$out

rule ld
  command = $CXX \$in -o \$out -lasmjit
  description = LD \$out

rule ruby
  command = ruby \$in \$out

build ./build/code.bin : ruby asm.rb | code.rb macros.rb
EOF

objs=()
shopt -s nullglob
for file in $(find src/ -type f -name '*.cpp'); do
  obj="build/${file#src/}.o"
  echo "build $obj : cxx $file" >>build.ninja
  objs+=("$obj")
done

echo build build/main : ld "${objs[@]}" >>build.ninja

ninja

rm -f /tmp/dump_*.bin

./build/main ./build/code.bin "$@"
