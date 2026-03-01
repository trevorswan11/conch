{
  description = "Conch language development.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            zig
            zls
            llvmPackages_21.clang-tools
            llvmPackages_21.lldb
          ];

          # Without this hook, Zig freaks out over unknown flags
            shellHook = ''
              export NIX_CFLAGS_COMPILE=$(echo $NIX_CFLAGS_COMPILE | sed 's/-fmacro-prefix-map=[^ ]*//g')
              export NIX_LDFLAGS=$(echo $NIX_LDFLAGS | sed 's/-fmacro-prefix-map=[^ ]*//g')
            '';
        };
      }
    );
}
