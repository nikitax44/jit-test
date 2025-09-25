#!/usr/bin/env bash
set -e
cat >build.ninja <<'EOF'
rule cxx
  command = g++ -std=c++20 -MMD -MF $out.d -c $in -o $out $cflags
  depfile = $out.d
  deps = gcc
  description = CXX $out

rule ld
  command = g++ $in -o $out -lkeystone
  description = LD $out
EOF

objs=()
shopt -s nullglob
for file in $(find src/ -type f -name '*.cpp'); do
  obj="build/${file#src/}.o"
  echo build "$obj" : cxx "$file" >>build.ninja
  objs+=("$obj")
done

echo build build/main : ld "${objs[@]}" >>build.ninja

ninja

./build/main "$@"
