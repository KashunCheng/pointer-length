{
  description = "A environment for cs201 project2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      with pkgs;
      {
        devShells.default = let llvm = llvmPackages_19.libllvm; in
          mkShell {
            buildInputs = [
              llvm
              graphviz
              cmake
              (python3.withPackages (python-pkgs: with python-pkgs; [
                pytest
              ]))
            ];
            shellHook =
              ''
                export LT_LLVM_INSTALL_DIR="${llvm.dev.outPath}"
              '';
          };
        formatter = nixpkgs-fmt;
      }
    );
}
