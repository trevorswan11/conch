<h1 align="center">ghoti</h1>

<p align="center">
<img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B&logoColor=white" alt="C++23" /> <img src="https://img.shields.io/badge/Zig-0.16.0-orange?logo=zig" alt="Zig 0.16.0" /> <a href="LICENSE"><img src="https://img.shields.io/github/license/trevorswan11/ghoti" alt="License" /></a> <a href="https://github.com/trevorswan11/ghoti/actions/workflows/format.yml"><img src="https://github.com/trevorswan11/ghoti/actions/workflows/format.yml/badge.svg" alt="Formatting" /></a> <a href="https://github.com/trevorswan11/ghoti/actions/workflows/ci.yml"><img src="https://github.com/trevorswan11/ghoti/actions/workflows/ci.yml/badge.svg" alt="CI" /></a> <img src="https://raw.githubusercontent.com/trevorswan11/ghoti/coverage/coverage.svg" alt="Coverage" />
</p>

<p align="center">
A hand-crafted systems programming language.
<br />
<a href="https://github.com/trevorswan11/ghoti/tree/main/doc"><strong>Explore the docs »</strong></a>
<br />
<br />
<a href="https://github.com/trevorswan11/ghoti/issues/new?labels=bug&template=bug-report.md">Report Bug</a>
&middot;
<a href="https://github.com/trevorswan11/ghoti/issues/new?labels=enhancement&template=feature-request.md">Request Feature</a>
</p>

## About the Project

Ghoti is a compiled systems language powered by LLVM, C++, and Zig. It attempts to combine select features from its more popular predecessors (i.e. Zig, Rust, C, C++) into a performant low-level language.

### What's With the Name?

The name 'ghoti' was largely inspired by VSauce's short about [forbidden spellings](https://youtube.com/shorts/3ipFdRfFvK4?si=0cdgxtmpbaFZtFHM). With the help of some abuse of the English language, **'ghoti'** is pronounced **'fish'**:
- gh pronounced as the /f/ in enough
- o pronounced as the /ɪ/ in women
- ti pronounced as the /ʃ/ in nation

### Built With Zig!

Zig is used as the primary orchestrator for all things ghoti. Ghoti uses Zig's `build.zig` to provide a hermetic build. Necessary dependencies are automatically fetched and all required dependencies are built from source. This unified build system manages LLVM compilation (including tools like clang-format), kcov coverage reporting (on supported platforms), and core maintainer tools such as a custom archiver for releases. Ghoti aims to be reproducible anywhere that has a valid and correctly versioned Zig. **No manual linking or hoop-jumping is required to build ghoti, ever, on any platform**.

<details>
<summary><b>Full dependency breakdown</b></summary>

The following are "standalone" dependencies, required and manually fetched by ghoti's build system.
1. [Catch2](https://github.com/catchorg/Catch2)'s amalgamated source code is compiled from source for test running. It is automatically configured in the project's build script and links statically to the test builds.
2. [cppcheck](https://cppcheck.sourceforge.io/) is compiled from source for static analysis. It is licensed under the GNU GPLv3, but the associated compiled artifacts are neither linked with output artifacts nor shipped with releases.
3. [magic_enum](https://github.com/Neargye/magic_enum) is used as a utility to reflect on enum values. Is is licensed under the permissive MIT License.
4. [fmt](https://github.com/fmtlib/fmt) is used as a formatting utility in place of std::format, which is not as performant or feature-full. Is is licensed under the permissive MIT License.
5. [unordered_dense](https://github.com/martinus/unordered_dense) provides a vastly improved hash map/set implementation that is used over the inefficient C++ standard implementation. Is is licensed under the permissive MIT License.
6. [gsl](https://github.com/microsoft/gsl) is used for enforcing best practices and supporting the standard template library
7. [CLI11](https://github.com/CLIUtils/CLI11) is a command line parser for C++ that provides a rich feature set with a simple and intuitive interface. Is is licensed under the permissive 3-Clause BSD License.
8. [LLVM 21.1.8](https://releases.llvm.org/21.1.0/docs/ReleaseNotes.html) is used as ghoti's compilation backend. It is manually compiled and statically linked against ghoti through the build system. It is licensed under the permissive Apache License 2.0, and has the following dependencies:
    - [libxml2](https://gitlab.gnome.org/GNOME/libxml2), licensed under the MIT License
    - [zlib](https://github.com/madler/zlib), licensed under the MIT License
    - [zstd](https://github.com/facebook/zstd), licensed under the BSD License
9. [libarchive](https://github.com/libarchive/libarchive) is used for packaging releases, making use of zlib and zstd to create `zip` and `zst` archives. It is license under the BSD 2-Clause License, but the associated compiled artifacts are neither linked with output artifacts nor shipped with releases.
10. [kcov](https://github.com/SimonKagstrom/kcov) is used for test coverage reporting. The licensing of this tool and its dependencies are not explicitly listed here as they are not shipped with releases of ghoti. It has multiple dependencies, but they are all fetched lazily as kcov is only supported on Linux, MacOS, and FreeBSD:
    - [curl](https://github.com/curl/curl) is required by all builds of kcov and is used for pulling the resulting badge. It has a single extra dependency which is chosen for cross-platform support:
        - [mbedtls](https://github.com/Mbed-TLS/mbedtls)
    - [binutils](https://sourceware.org/pub/binutils) is required for all kcov builds
    - [elfutils](https://github.com/Techatrix/elfutils) is required on linux only. It has a single extra dependency:
        - [argp-standalone](https://github.com/argp-standalone/argp-standalone)
    - [libdwarf-code](https://github.com/davea42/libdwarf-code) is required on MacOS only.

Many build functions heavily reference [allyourcodebase](https://github.com/allyourcodebase)'s implementations. Links to specific repositories can be found as a documentation comment above respective `build` functions.

</details>

### Core Principles
- Learn for the sake of learning
- Experiment freely
- KISS & DRY

Development speed is _not_ and _will never be_ a core principle of ghoti.

### Hello, World!
```ghoti
import std;

pub const main := fn(args: [][:0]u8): void {
    const message := "Hello, world!";
    std::io::println(message);
};
```

## Getting Started
### For Nix Users
This is by far the easiest way to get started with development. Just run `nix develop` to get started and automatically get the correct Zig version and some important development tools. Note that this provides optional preconfigured tools such as LLDB, Clangd, and ZLS to further enhance the developer experience.

### For Others
All you need to get started with ghoti development is git and a valid 0.16.0 Zig installation, which can be found [here](https://ziglang.org/download/).

In either case, assuming you have the git and Zig prerequisites on your system, building ghoti is as easy as running:
```sh
git clone https://github.com/trevorswan11/ghoti
cd ghoti
zig build --release
```

## Roadmap

- [x] Lexical analysis
- [x] Pratt parsing
    - [x] Syntax documentation
- [ ] Multi-pass Semantic Analysis (to support order independent declarations)
    - [x] Symbol registration pass
    - [x] Type resolution pass
    - [ ] Type checking pass
- [ ] LLVM Integration
    - [ ] Build system integration
        - [x] Compilation rules for Clang, LLD, and LLVM
        - [x] In-house clang-format
        - [x] Kaleidoscope examples
        - [ ] Test parity through the build system
    - [ ] Compiler backend integration

See the [open issues](https://github.com/trevorswan11/ghoti/issues) for a full list of proposed features (and known issues).

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are greatly appreciated.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement". Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feat/AmazingFeature`)
3. Commit your Changes (`git commit -m '[feat]: Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feat/AmazingFeature`)
5. Open a Pull Request against `dev`

### Top contributors:

<a href="https://github.com/trevorswan11/ghoti/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=trevorswan11/ghoti" alt="contrib.rocks image" />
</a>

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Contact

[![LinkedIn](https://img.shields.io/badge/linkedin-%230077B5.svg?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/trevorswan11/) [![Gmail](https://img.shields.io/badge/Gmail-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:trevor.swan@case.edu)

Project Link: [https://github.com/trevorswan11/ghoti](https://github.com/trevorswan11/ghoti)

## Acknowledgments

- [Zig](https://ziglang.org/)'s community, language features, and compiler source code
- [Rust](https://rust-lang.org/)'s language features and philosophy
- [cppreference](https://www.cppreference.com/)'s extensive C++ language documentation
- [Thorsten Ball](https://thorstenball.com/)'s "Writing an Interpreter/Compiler in Go" two-book series
