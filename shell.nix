{pkgs ? import <nixpkgs> {}}:
let
  # pkgsCross= pkgs.pkgsCross.mingwW64;
  pkgsCross = pkgs;
in
pkgsCross.mkShell {
  nativeBuildInputs = [
    pkgs.ninja
    pkgs.ruby
  ];
  buildInputs = [
    pkgsCross.asmjit
  ];
}
