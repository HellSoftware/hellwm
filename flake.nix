{
  description = "HellWM";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }: {
    packages = let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      default = pkgs.stdenv.mkDerivation {
        pname = "hellwm";
        version = "0.0.1";

        src = pkgs.lib.cleanSource ./.;

        buildInputs = with pkgs; [
          pixman
          wayland
          libinput
          pkg-config
          wlroots_0_18
          libxkbcommon
          wayland-scanner
          wayland-protocols
        ];

        buildPhase = ''
          make
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp hellwm $out/bin
        '';

        meta = with pkgs.lib; {
          description = "HellWM";
          homepage = "https://github.com/HellSoftware/HellWM";
          maintainers = [ "danihek" ];
        };
      };
    };

    defaultPackage.x86_64-linux = self.packages.default;
    defaultApp.x86_64-linux = {
      type = "app";
      program = "${self.packages.default}/bin/hellwm";
    };
  };
}
