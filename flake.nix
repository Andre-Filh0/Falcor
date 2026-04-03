{
  description = "Falcor — ambiente de desenvolvimento e cross-compilacao para Kria KR260";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs   = import nixpkgs { inherit system; };
    cross  = pkgs.pkgsCross.aarch64-multiplatform;

    # ---------------------------------------------------------------
    # quill v2.x — biblioteca de logging C++ (odygrd/quill)
    # O pkgs.quill do nixpkgs e uma ferramenta blockchain diferente.
    # Logger.hpp usa a API v2 (quill::start, quill::stdout_handler).
    # ---------------------------------------------------------------
    quill-cpp = pkgs.stdenv.mkDerivation rec {
      pname   = "quill-cpp";
      version = "2.9.1";
      src = pkgs.fetchFromGitHub {
        owner = "odygrd";
        repo  = "quill";
        rev   = "v${version}";
        hash  = "sha256-iCn0uOHFooEs9LEVC0UQsgPyuUKXLVXMzoYxKTmQuPQ=";
      };
      nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
      cmakeFlags = [
        "-DQUILL_BUILD_TESTS=OFF"
        "-DQUILL_BUILD_EXAMPLES=OFF"
        "-DQUILL_FMT_EXTERNAL=OFF"
      ];
      # O quill.pc gerado tem paths com // duplo no Nix (bug do upstream).
      # Usamos CMake config, nao pkg-config, entao o .pc nao e necessario.
      postInstall = "rm -rf $out/pkgconfig";
    };

    # ---------------------------------------------------------------
    # Mesmo quill cross-compilado para aarch64
    # ---------------------------------------------------------------
    quill-cpp-cross = cross.stdenv.mkDerivation rec {
      pname   = "quill-cpp";
      version = "2.9.1";
      src = pkgs.fetchFromGitHub {
        owner = "odygrd";
        repo  = "quill";
        rev   = "v${version}";
        hash  = "sha256-iCn0uOHFooEs9LEVC0UQsgPyuUKXLVXMzoYxKTmQuPQ=";
      };
      nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
      cmakeFlags = [
        "-DQUILL_BUILD_TESTS=OFF"
        "-DQUILL_BUILD_EXAMPLES=OFF"
        "-DQUILL_FMT_EXTERNAL=OFF"
      ];
      postInstall = "rm -rf $out/pkgconfig";
    };

    buildTools = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];

    nativeLibs = [
      pkgs.zeromq
      pkgs.cppzmq
      quill-cpp
      pkgs.gst_all_1.gstreamer
      pkgs.gst_all_1.gst-plugins-base
      pkgs.gst_all_1.gst-plugins-good
      pkgs.gst_all_1.gst-plugins-bad
    ];

    crossLibs = [
      cross.zeromq
      cross.cppzmq
      quill-cpp-cross
      cross.gst_all_1.gstreamer
      cross.gst_all_1.gst-plugins-base
      cross.gst_all_1.gst-plugins-good
    ];

  in {
    devShells.${system} = {

      # Desenvolvimento e testes no PC (x86_64 nativo)
      # Uso: nix develop
      default = pkgs.mkShell {
        nativeBuildInputs = buildTools;
        buildInputs       = nativeLibs;

        shellHook = ''
          export CMAKE_PREFIX_PATH="${pkgs.zeromq}:${pkgs.cppzmq}:${quill-cpp}:${pkgs.gst_all_1.gstreamer.dev}:${pkgs.gst_all_1.gst-plugins-base.dev}:$CMAKE_PREFIX_PATH"
          echo "╔══════════════════════════════════════╗"
          echo "║  Falcor — PC dev (x86_64 nativo)     ║"
          echo "╚══════════════════════════════════════╝"
          echo "cmake : $(cmake --version | head -1)"
          echo "ninja : $(ninja --version)"
          echo "gcc   : $(gcc --version | head -1)"
          echo ""
          echo "Build: cmake --preset linux-x64-debug && cmake --build out/build/linux-x64-debug"
        '';
      };

      # Cross-compilacao para Kria KR260 (aarch64)
      # Uso: nix develop .#kria
      kria = pkgs.mkShell {
        nativeBuildInputs = buildTools ++ [
          cross.stdenv.cc
          cross.stdenv.cc.bintools
        ];
        buildInputs = crossLibs;

        shellHook = ''
          export CMAKE_PREFIX_PATH="${cross.zeromq}:${cross.cppzmq}:${quill-cpp-cross}:$CMAKE_PREFIX_PATH"
          export CMAKE_TOOLCHAIN_FILE="$(pwd)/cmake/toolchains/kriakr260.cmake"
          echo "╔══════════════════════════════════════╗"
          echo "║  Falcor — Kria KR260 (aarch64 cross) ║"
          echo "╚══════════════════════════════════════╝"
          echo "Build: cmake --preset linux-kria-release && cmake --build out/build/linux-kria-release"
        '';
      };

    };
  };
}
