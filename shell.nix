{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  packages = [
    pkgs.asmjit
    pkgs.ninja
    pkgs.ruby
  ];
}
