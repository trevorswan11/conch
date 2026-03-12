# Types
## Explicit Types
- Types in conch are first class values and can be assigned to values and passed to functions
- There are 4 different explicit type modifiers in conch
    - `*`: Denotes a constant pointer
    - `*mut`: Denotes a mutable pointer
    - `&`: Denotes a constant reference
    - `&mut`: Denotes a mutable reference
- These modifiers act in an opposite way to languages like C, C++, and Zig, while behaving similar to Rust
    - Mutability is opt-in, with everything else being constant by default
- There are 4 different explicit types in conch
    - Identifier types: Consist of an identifier (either a keyword or user defined type)
    - Function types: Consist of the functions argument types and return type.
    - Array types: Consist of an optional size (required if array, not if slice) and type which can be any of these types. These are recursively defined.
    - Nested types: Consist of an outer type modifier acting on any of these types in a recursive fashion
- The location of the type modifier is extremely important:
```conch
*mut [5uz]int // A mutable pointer to an array of 5 integers.
[5uz]*mut int // A constant array of 5 mutable pointers to integers
```
- Each of these explicit types can be accompanied by any amount of type modifiers (syntactically)
    `&&&&&&&&&&&&&&&int` is technically a valid type, though it likely would never be used
- Types can be passed to functions whose parameter has the type `type`
    - Function argument parsing prioritizes expressions over types
```conch
const func := fn(p: type): void { ... };
func(&mut P);
```
- In the above example, the call expression is parsed as taking in a mutable reference to a declaration P
- If you instead meant for `&mut P` to be a type, you can circumvent this priority by using a `using` statement
```conch
using tP = &mut P;
const func := fn(p: type): void { ... };
func(tP);
```
- In this example, the identifier `tP` is aliased to a type which is a mutable reference to a type `P`
- Types can also be returned from functions, assuming the type can be fully constructed at compile time
- Function types require parameter names for clarity and maintainability:
```conch
using Callback = fn(status: int, result: *bool): void;
const register := fn(cb: Callback): void { ... };
```

## Pointers vs. References
- When possible, references should be used over pointers
- While references and pointers act similarly, references are guaranteed to be non-null
- Any pointer type can receive the builtin `nullptr` pointer
    - This is the only use-case for a built-in null in conch
- A reference to an object can access its internal fields using the dot operator
- You can reference a pointer by using the `*` operator
- A pointer to an object can access its internal fields using the arrow (`->`) operator
    - Note that this is just syntactic sugar for an explicit dereference expression
```conch
a->b;   // Syntactic sugar
(*a).b; // Equivalent expression
```

## Type Introspection
- You can get the type of a value by using the `@typeOf` builtin
- You can get the size of a type (in bytes) by using the `@sizeOf` builtin
- You can get the align of a type by using the `@alignOf` builtin

## Primitive Types
- `int`: A 32-bit signed integer, represented by a standalone number
- `long`: A 64-bit signed integer, represented by a `l` suffixed number
- `isize`: A platform specific signed integer equivalent to ptrdiff_t (64-bit on 64-bit systems), represented by a `z` suffixed number
- `uint`: A 32-bit unsigned integer, represented by a `u` suffixed number
- `ulong`: A 64-bit unsigned integer, represented by a `ul` suffixed number
- `usize`: A platform specific unsigned integer equivalent to size_t (64-bit on 64-bit systems), represented by a `uz` suffixed number
- `float`: A 32-bit floating point, represented by a `f` suffixed number, decimal, or scientific notation
- `double`: A 64-bit floating point, represented by a standalone decimal or scientific notation
- `byte`: An unsigned 8-bit value, typically used for characters (i.e. `'a'`) or ascii values
- `string`: A compile time (static storage duration) string constant consisting of a size and data which is an array of bytes.
- `bool`: True or false
- `void`: The unit type
