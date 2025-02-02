# Project 2: Local Value Numbering (LVN)

![tests](https://github.com/KashunCheng/cs201-project2/actions/workflows/test.yml/badge.svg)

## Introduction

This project implements a basic LLVM analysis pass for Local Value Numbering (LVN). The LVN pass identifies redundant
arithmetic computations in a function. It does not remove the redundancies but simply annotates the LLVM IR with value
numbers and flags redundant operations.

### Requirements

+ LLVM 19: A valid LLVM 19 installation is required.
+ Clang 19: The Clang 19 compiler is needed to compile test cases.
+ C++ Compiler: Must support C++20. On Linux, use g++-14 or later. On macOS, use Clang 19 or later.
+ CMake: For building the project.
+ Python3 & Pytest: For running automated tests (provided in the test/ directory).
+ Nix (Optional): You can use Nix along with the provided flake.nix to create a reproducible development environment. In
  the Nix shell, all required dependencies (LLVM 19, Clang, CMake, GCC, Python3, and Pytest) are provided, and the
  `LT_LLVM_INSTALL_DIR` variable is automatically set. The compilation process remains the same.

### Building the Project

Before building, set the `LT_LLVM_INSTALL_DIR` variable to the root of your LLVM installation.

#### Example for Linux/Mac:

```bash
# For Nix users
nix develop
# For non-Nix users
export LT_LLVM_INSTALL_DIR=<path-to-your-llvm-installation>

# Build the project (these steps are the same for Nix and non-Nix users)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

The shared library for the LVN pass (for example, `libHelloWorld.so` on Linux or `libHelloWorld.dylib` on macOS) will be
created in the build output directory (typically under build/lib).

### Running and Testing the LVN Pass

#### Run the LVN Pass with Opt

Run your LVN pass using opt as follows:

```bash
opt -load-pass-plugin ./libHelloWorld.{so|dylib} -passes=hello-world -disable-output test.ll
```

The output (printed to stderr) should show each LLVM IR instruction along with the corresponding value numbers.
Redundant computations should be clearly labeled.

#### Automated Testing with Pytest

A Python script (test/test_pass.py) is provided to run all test cases. To execute the tests, simply run:

```bash
pytest
```

This script:

+ Compiles each .c test file into LLVM IR.
+ Runs the LVN pass via opt with the proper shared library.
+ Compares the output against the expected results in the corresponding .out files.

The GitHub Actions workflow (see `.github/workflows/test.yml`) will also run these tests automatically on each push or
pull request.
