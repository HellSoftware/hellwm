let
  unstable = import (fetchTarball https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz) { };
in
{ nixpkgs ? import <nixpkgs> {} }:
with nixpkgs; mkShell {
    name = "HellWM env";
    
    nativeBuildInputs = [
    lua
    gdb
    udev
    clang
    seatd 
    libGL
    libGLU
    pixman
    libgcc
    glxinfo
    gnumake
    libllvm
    llvm_18
    wayland
    libinput 
    pkg-config
    xorg.libxcb
    libxkbcommon
    xorg.xcbutilwm
    wayland-scanner
    wayland-protocols
    unstable.wlroots_0_18
  ];

  buildInputs = [];
}
