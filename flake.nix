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
      with pkgs;
      {
        devShells.default = mkShell {
          hardeningDisable = [ "all" ];
          buildInputs = [
            zig_0_15
            zls_0_15
            llvmPackages_21.clang-tools
            llvmPackages_21.lldb
          ];

          # Required for LLDB on macOS (stinky)
          shellHook = lib.optionalString stdenv.isDarwin ''
            # Tested both paths on my machine and they both work (adds some flexibility)
            if [[ -z "$LLDB_DEBUGSERVER_PATH" ]]; then
              XCODE_PATH="/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Versions/A/Resources/debugserver"
              CLT_PATH="/Library/Developer/CommandLineTools/Library/PrivateFrameworks/LLDB.framework/Versions/A/Resources/debugserver"

              if [[ -f "$XCODE_PATH" ]]; then
                export LLDB_DEBUGSERVER_PATH="$XCODE_PATH"
              elif [[ -f "$CLT_PATH" ]]; then
                export LLDB_DEBUGSERVER_PATH="$CLT_PATH"
              fi
            fi
          '';
        };
      }
    );
}
