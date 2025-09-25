{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  packages = [
    pkgs.keystone
    pkgs.ninja
    pkgs.ruby
  ];
}
