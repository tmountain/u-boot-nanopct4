{ pkgs ? import <nixpkgs> {} }:

let
  nixpkgs = import pkgs.path { system = "x86_64-linux"; };
  board = "NanoPC-T4";
in
nixpkgs.stdenv.mkDerivation {
  name = "armbian-rkbin-${board}";

  src = nixpkgs.fetchFromGitHub {
    rev = "3bd0321cae5ef881a6005fb470009ad5a5d1462d";
    owner = "armbian";
    repo = "rkbin";
    sha256 = "09r4dzxsbs3pff4sh70qnyp30s3rc7pkc46v1m3152s7jqjasp31";
  };

  nativeBuildInputs = [ nixpkgs.patchelf nixpkgs.makeWrapper ];

  dontBuild = true;
  dontStrip = true;

  installPhase = ''
    TOOLS=(
      tools/firmwareMerger
      tools/kernelimage
      tools/loaderimage
      tools/mkkrnlimg
      tools/rkdeveloptool
      tools/trust_merger
    )

    for tool in "''${TOOLS[@]}"; do
      echo "Patching $tool"
      patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" "$tool"
    done

    # ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked
    TOOLS+=(tools/resource_tool)

    mkdir -p $out/share/rkbin
    for f in *; do
      cp -r $f $out/share/rkbin/
    done

    for tool in "''${TOOLS[@]}"; do
      makeWrapper $out/share/rkbin/$tool $out/bin/''${tool/tools/}
    done
  '';

  meta = with nixpkgs.stdenv.lib; {
    description = "Proprietary bits from Rockchip used by Rockchip for ${board}";
    license = licenses.unfree;
  };
}
