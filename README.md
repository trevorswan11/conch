<h1 align="center">🐚conch🐚</h1>

<p align="center">
<img src="https://img.shields.io/badge/C-17-blue?logo=&logoColor=white" alt="C17" /> <a href="LICENSE"><img src="https://img.shields.io/github/license/trevorswan11/conch" alt="License" /></a> <img src="https://img.shields.io/github/last-commit/trevorswan11/conch" alt="Last Commit" /> <a href="https://github.com/trevorswan11/conch/actions/workflows/format.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/format.yml/badge.svg" alt="Formatting" /></a> <a href="https://github.com/trevorswan11/conch/actions/workflows/ci.yml"><img src="https://github.com/trevorswan11/conch/actions/workflows/ci.yml/badge.svg" alt="CI" /></a> <a href="https://github.com/trevorswan11/conch/blob/coverage/coverage_summary.txt"><img src="https://raw.githubusercontent.com/trevorswan11/conch/coverage/coverage.svg" alt="Coverage" /></a>
</p>

<p align="center">
A programming language written in C.
</p>

# Motivation
This project is a revamp of [zlx](https://github.com/trevorswan11/zlx). Due to some upcoming coursework in compiler design, I wanted to take on this type of project with two main goals:
- Become proficient in the C programming language _without_ the help of AI or external libraries (other than Catch2 for testing)
- Fully understand programming concepts relating to the internals of an interpreter, bytecode VM, and compiler

ZLX was a fun project and got me into Low-Level programming, but its design choices limited its extensibility. I hope to use this project to grow as a developer and as a problem solver, expanding my knowledge of core programming concepts and data structures. 

# Getting Started
System dependencies:
1. CMake (3.20 or later)
2. Any mainstream C compiler (LLVM Clang is recommended for maximum compatibility)
3. Clang-format

Once these are installed, building conch is as easy as running:
```sh
git clone https://github.com/trevorswan11/conch
cd conch
mkdir build
cd build
cmake ..
cmake --build . --target dist
```

This builds the `dist` configuration, enabling maximum optimization and disabling assertions and debug symbols. 

# Correctness & Availability
[Catch2](https://github.com/catchorg/Catch2) is used with [C++20](https://en.cppreference.com/w/cpp/20.html) to run automated CI tests on Windows (MSVC & msys2), macOS (LLVM Clang), and Linux (GCC & Clang). This choice allows me to take advantage of the rich C++ ecosystem and standard library while prioritizing correctness in the C code. 

As I cannot run hundreds of matrix tests, I am unable to verify support for arbitrary platforms. Please let me know if there's something I can do to make the project more widely available.

# Resources
This project would not be possible without the extensive work Thorsten Ball put into his two-book series, "Writing an Interpreter and Compiler in Go". The explanations presented in these books drove this project's development at a high level and greatly enhanced my learning. If you want to check out the books for yourself and support Thorsten, check him out [here](https://store.thorstenball.com/).

It is worth noting that this book was __not__ used as a direct guide. I deviated heavily from Thorsten's implementation in many areas, effectively testing my knowledge of C and understanding of core concepts along the way.
