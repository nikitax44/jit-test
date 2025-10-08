#!/usr/bin/env bash
set -e
CXX="$(which "$CXX")" # to inform ninja that 32-bit g++ and 64-bit g++ are different programs
machine="$($CXX -dumpmachine)"
if [[ "$machine" =~ (mingw) ]]; then
  outfile="build/main.exe"
  runner=wine
else
  outfile="build/main"
  runner=
fi

cat >build.ninja <<EOF
cflags = $CFLAGS -std=c++20 -O3 -g3
asflags =
rule cxx
  command = $CXX \$cflags -MMD -MF \$out.d \$in -o \$out -c
  depfile = \$out.d
  deps = gcc
  description = CXX \$out

rule as
  command = $CXX \$asflags -c \$in -o \$out
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

for file in $(find src/ -type f -name '*.S'); do
  obj="build/${file#src/}.o"
  echo "build $obj : as $file" >>build.ninja
  objs+=("$obj")
done

echo "build $outfile : ld ${objs[@]}" >>build.ninja

ninja

rm -f /tmp/dump_*.bin

$runner $outfile ./build/code.bin "$@"
