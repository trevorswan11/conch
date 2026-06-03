# Style Guide

Ghoti adheres to a somewhat strict style guide to enhance readability.

## Formatting
`clang-format` is used for code formatting all C++ code, while Zig's builtin formatter is used for formatting all Zig code. While the latter of the two comes for free with a proper Zig installation, the former can be quite involved. In order to invoke the tool automatically, ghoti's build system exposes a step executed with `zig build fmt`. This is incredibly resource intensive, building the entire `clang-format` tool from scratch (though it does cache the binary). If you do not have the compute to front this cost, that is perfectly acceptable. A maintainer will be more than happy to format your changes on your behalf using their system, eliminating any worry of PR rejection.

Upon PR creation and workflow approval, GitHub actions will run `zig build fmt-check`, an equally-intensive step that performs a dry run of `clang-format` on the codebase. This can also be invoked on your machine if you are interested in confirming the state of your local version of the codebase.

## General Naming Conventions
- **Files**: `snake_case.cc`, `snake_case.hh`, `snake_case.zig`
- **Functions & Variables**: snake_case
- **Types (Classes, Structs, Enums)**: PascalCase
- **Type Traits**: snake_case (e.g. mem::is_box)
- **Template Parameters**: PascalCase (e.g., template <typename TValue, bool Real>)
- **Constants & Macros**: SCREAMING_SNAKE_CASE
- **Private/Internal Members**: Suffix with an underscore (e.g. member_variable_)

## Language-Specific Conventions

### C++
- Use `.hh` for headers and `.cc` for source files
- Avoid `using namespace ...;` in headers
- Prefer enum class over standard enum
- Use the `#pragma once` directive over `#ifdef` include guards
- `#undef` macros in header files after use when possible
- All files should include only what they use and should avoid transitive includes wherever possible

### Zig
- Use PascalCase for functions that return a type
- Use PascalCase for files that should be treated as types
- Follow the [Zig Standard Library style](https://ziglang.org/documentation/0.16.0/#Style-Guide)
