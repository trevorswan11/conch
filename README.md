<h1 align="center">conch</h1>

<p align="center">
<img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B&logoColor=white" alt="C++23" /> <a href="LICENSE"><img src="https://img.shields.io/github/license/trevorswan11/conch" alt="License" /></a> <img src="https://img.shields.io/github/last-commit/trevorswan11/conch" alt="Last Commit" /> <a href="https://github.com/trevorswan11/conch/actions/workflows/format.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/format.yml/badge.svg" alt="Formatting" /></a> <a href="https://github.com/trevorswan11/conch/actions/workflows/ci.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/ci.yml/badge.svg" alt="CI" /></a>
</p>

<p align="center">
A programming language.
</p>

# Motivation
This project is a revamp of [zlx](https://github.com/trevorswan11/zlx). Due to some upcoming coursework in compiler design, I wanted to take on this type of project with two main goals:
- Become proficient in the C++ programming language _without_ the help of AI
- Fully understand programming concepts relating to the internals of an interpreter, bytecode VM, and compiler

ZLX was a fun project and got me into Low-Level programming, but its design choices limited its extensibility. I hope to use this project to grow as a developer and as a problem solver, expanding my knowledge of core programming concepts and data structures.

# Getting Started
## System dependencies:
1. [Zig 0.15.2](https://ziglang.org/download/) drives the build system, including artifact compilation, libcpp includes, and project tooling.
2. [LLVM 21.x](https://releases.llvm.org/21.1.0/docs/ReleaseNotes.html) is used as Conch's compilation backend. It is required for building Conch from scratch, but its libraries are statically linked in resulting executables. `llvm-config` must be available in the system's PATH so that the build system knows where to find library includes and compiled artifacts. It is licensed under the permissible Apache License 2.0.
    - On MacOS, I use the nix package manager to manage LLVM. You will likely find success using homebrew, though I cannot personally verify this.
    - On Windows, I use MSYS2 to manage the UCRT LLVM package, found [here](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-llvm). I do not recommend taking another path than this, but you may find another installation method works.
    - On Linux, it should be trivial for you to install LLVM on your system, either with [precompiled releases](https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.8) or through your package manager.

The easiest way to get started with development is with [nix](https://nixos.org/). Just run `nix develop` to get started. Otherwise, you must manually install required dependencies in a way that fits your specific system.

## Other dependencies:
### General
The following are "standalone" dependencies, only required by conch itself.
1. [Catch2](https://github.com/catchorg/Catch2)'s amalgamated source code is compiled from source for test running. It is automatically configured in the project's build script and links statically to the test builds.
2. [cppcheck](https://cppcheck.sourceforge.io/) is compiled from source for static analysis. It is licensed under the GNU GPLv3, but the associated compiled artifacts are neither linked with output artifacts nor shipped with releases.
3. [magic_enum](https://github.com/Neargye/magic_enum) is used as a utility to reflect on enum values. Is is licensed under the permissible MIT license.

### LLVM-Specific
The following are dependencies of LLVM and are compiled from source. They are linked statically against conch and comply with the project's license.
1. [libxml2](https://gitlab.gnome.org/GNOME/libxml2)
2. [zlib](https://github.com/madler/zlib)

These are automatically downloaded by the zig build system, so once you're system dependencies are properly configured, building conch is as easy as running:
```sh
git clone https://github.com/trevorswan11/conch
cd conch
zig build --release
```

This builds the `ReleaseFast` configuration. You can read about Zig's different optimization levels [here](https://ziglang.org/documentation/master/#Build-Mode).

## Tooling Dependencies
1. [clang-format](https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.8) is used for C++ code formatting. If you have LLVM installed on your system (specifically the LLVM version required by Conch), then this is a trivial dependency. LLVM 21's formatter is used on all development platforms.
5. [zip](https://infozip.sourceforge.net/Zip.html) and [tar](https://www.gnu.org/software/tar/tar.html) are both used for packaging releases, but this is automated by GitHub actions runners.

Note that these dependencies are purely optional for users simply building the project from source!

# Correctness & Availability
[Catch2](https://github.com/catchorg/Catch2) is used with a custom [Zig](https://ziglang.org/) allocator to run automated CI tests on Windows, macOS, and Linux. This choice allows me to take advantage of the best-in-class testing suite provided by catch2 while making use of the undefined behavior and leak sanitizers provided by Zig and its build system.

As I cannot run hundreds of matrix tests, I am unable to verify support for arbitrary platforms. Please let me know if there's something I can do to make the project more widely available. While releases have prebuilt binaries for a myriad of systems, I cannot verify that they all work as intended out of the box. In the event that a release is shipped with a faulty binary, please open an issue!

# Resources
This project would not be possible without the extensive work Thorsten Ball put into his two-book series, "Writing an Interpreter and Compiler in Go". The explanations presented in these books drove this project's development at a high level and greatly enhanced my learning. If you want to check out the books for yourself and support Thorsten, check him out [here](https://store.thorstenball.com/).

It is worth noting that this book was __not__ used as a direct guide. I deviated heavily from Thorsten's implementation in many areas, effectively testing my knowledge of C and understanding of core concepts along the way.
