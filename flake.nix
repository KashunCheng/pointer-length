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
        devShells.default =
          let
            clang = llvmPackages_19.clang;
            gcc = gcc14;
            llvm = llvmPackages_19.libllvm;
          in
          mkShell {
            buildInputs = [
              clang
              cmake
              llvm
              (python3.withPackages (python-pkgs: with python-pkgs; [
                pytest
              ]))
            ] ++ lib.optional stdenv.isLinux gcc;
            shellHook =
              ''
                export LT_LLVM_INSTALL_DIR="${llvm.dev.outPath}"
              '' + (if stdenv.isLinux then ''
                export CC="${gcc}/bin/gcc"
                export CXX="${gcc}/bin/g++"
              '' else ''
                export CC="${clang}/bin/clang"
                export CXX="${clang}/bin/clang++"
              '');
          };
        formatter = nixpkgs-fmt;
      }
    );
}
