<h1 align="center">conch</h1>

<p align="center">
<img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B&logoColor=white" alt="C++23" /> <a href="LICENSE"><img src="https://img.shields.io/github/license/trevorswan11/conch" alt="License" /></a> <img src="https://img.shields.io/github/last-commit/trevorswan11/conch" alt="Last Commit" /> <a href="https://github.com/trevorswan11/conch/actions/workflows/format.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/format.yml/badge.svg" alt="Formatting" /></a> <a href="https://github.com/trevorswan11/conch/actions/workflows/ci.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/ci.yml/badge.svg" alt="CI" /></a><img src="https://raw.githubusercontent.com/trevorswan11/conch/badges/coverage.svg" alt="Coverage" />
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

The easiest way to get started with development is with [nix](https://nixos.org/). Just run `nix develop` to get started. Otherwise, you must manually install required dependencies in a way that fits your specific system.

## Other dependencies:
The following are "standalone" dependencies, required and manually fetched by Conch's build system.
1. [Catch2](https://github.com/catchorg/Catch2)'s amalgamated source code is compiled from source for test running. It is automatically configured in the project's build script and links statically to the test builds.
2. [cppcheck](https://cppcheck.sourceforge.io/) is compiled from source for static analysis. It is licensed under the GNU GPLv3, but the associated compiled artifacts are neither linked with output artifacts nor shipped with releases.
3. [magic_enum](https://github.com/Neargye/magic_enum) is used as a utility to reflect on enum values. Is is licensed under the permissible MIT license.
4. [fmt](https://github.com/fmtlib/fmt) is used as a formatting utility in place of std::format, which is not as performant or feature-full. Is is licensed under the permissible MIT license.
5. [LLVM 21.1.8](https://releases.llvm.org/21.1.0/docs/ReleaseNotes.html) is used as Conch's compilation backend. It is manually compiled and statically linked against conch through the build system. It is licensed under the permissible Apache License 2.0, and has the following dependencies:
    - [libxml2](https://gitlab.gnome.org/GNOME/libxml2)
    - [zlib](https://github.com/madler/zlib)
    - [zstd](https://github.com/facebook/zstd)
6. [libarchive](https://github.com/libarchive/libarchive) is used for packaging releases, making use of zlib and zstd to create `zip` and `zst` archives. It is license under the BSD 2-Clause License, but the associated compiled artifacts are neither linked with output artifacts nor shipped with releases.

These are automatically downloaded by the zig build system, so building conch is as easy as running:
```sh
git clone https://github.com/trevorswan11/conch
cd conch
zig build --release
```

This builds the `ReleaseFast` configuration. You can read about Zig's different optimization levels [here](https://ziglang.org/documentation/master/#Build-Mode).

# Correctness & Availability
[Catch2](https://github.com/catchorg/Catch2) is used with a custom [Zig](https://ziglang.org/) allocator to run automated CI tests on Windows, macOS, and Linux. This choice allows me to take advantage of the best-in-class testing suite provided by catch2 while making use of the undefined behavior and leak sanitizers provided by Zig and its build system.

As I cannot run hundreds of matrix tests, I am unable to verify support for arbitrary platforms. Please let me know if there's something I can do to make the project more widely available. While releases have prebuilt binaries for a myriad of systems, I cannot verify that they all work as intended out of the box. In the event that a release is shipped with a faulty binary, please open an issue!
