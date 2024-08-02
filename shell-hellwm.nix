let
  unstable = import (fetchTarball https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz) { };
in
{ nixpkgs ? import <nixpkgs> {} }:
with nixpkgs; mkShell {
    name = "HellWM env";
    
    nativeBuildInputs = [
    lua
    pixman
    wayland
    pkg-config
    xorg.libxcb
    libxkbcommon
    wayland-scanner
    wayland-protocols
    unstable.wlroots_0_18
  ];
  buildInputs = [ ];
}
