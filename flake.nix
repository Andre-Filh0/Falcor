{
  description = "Falcor KR260 build env";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";

    pkgs = import nixpkgs {
      inherit system;
      crossSystem = {
        config = "aarch64-linux-gnu";
      };
    };
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        cmake
        ninja
        pkg-config

        zeromq
        quill

        aarch64-linux-gnu-gcc
        aarch64-linux-gnu-binutils
      ];

      shellHook = ''
        echo "Ambiente Falcor KR260 carregado"
      '';
    };
  };
}