{ pkgs ? import <nixpkgs> { }, ... }:

pkgs.mkShell {
  name = "HellWM";
  packages = with pkgs; [
    pixman
    wayland
    libinput
    pkg-config
    wlroots_0_18
    libxkbcommon
    wayland-scanner
    wayland-protocols
  ];
}
