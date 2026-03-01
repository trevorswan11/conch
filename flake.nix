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
          hardeningDisable = [ "all" ];
          buildInputs = with pkgs; [
            zig_0_15
            zls_0_15
            llvmPackages_21.clang-tools
            llvmPackages_21.lldb
          ];
        };
      }
    );
}
