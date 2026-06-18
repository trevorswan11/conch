# Types
## Explicit Types
- Types in ghoti are first class values and can be assigned to values and passed to functions
- There are 5 different explicit type modifiers in ghoti
    - `^`: Denotes a constant pointer
    - `^mut`: Denotes a mutable pointer
    - `&`: Denotes a constant reference
    - `&mut`: Denotes a mutable reference
    - `volatile`: Denotes a [volatile](https://en.wikipedia.org/wiki/Volatile_(computer_programming)) value, preventing certain compiler optimizations for the sake of correctness
- Most of these modifiers act in an opposite way to languages like C, C++, and Zig, while behaving similar to Rust
    - Mutability is opt-in, with everything else being constant by default
- There are 4 different explicit types in ghoti
    - Identifier types: Consist of an identifier (either a keyword or user defined type)
    - Function types: Consist of the functions argument types and return type.
    - Array types: Consist of an optional size (required if array, not if slice) and type which can be any of these types. These are recursively defined.
        - You can annotate an array type to indicate that it is null-terminated by suffixing the size with `:0` (i.e. `[N:0]`)
        - You can annotate a slice type to indicate that it is null-terminated by placing `:0` in between the brackets (i.e. `[:0]`)
    - Nested types: Consist of an outer type modifier acting on any of these types in a recursive fashion
- The location of the type modifier is extremely important:
```ghoti
^mut [5uz]i32 // A mutable pointer to an array of 5 integers.
[5uz]^mut i32 // A constant array of 5 mutable pointers to integers
```
- Each of these explicit types can be accompanied by any amount of type modifiers (syntactically)
    `&&&&&&&&&&&&&&&i32` is technically a valid type, though it likely would never be used
- Types can be passed to functions whose parameter has the type `type`
    - Function argument parsing prioritizes expressions over types
```ghoti
const func := fn(p: type): void { ... };
func(&mut P);
```
- In the above example, the call expression is parsed as taking in a mutable reference to a declaration P
- If you instead meant for `&mut P` to be a type, you can circumvent this priority by using a `using` statement
```ghoti
using tP = &mut P;
const func := fn(p: type): void { ... };
func(tP);
```
- In this example, the identifier `tP` is aliased to a type which is a mutable reference to a type `P`
- Types can also be returned from functions, assuming the type can be fully constructed at compile time
- Function types require parameter names for clarity and maintainability:
```ghoti
using Callback = fn(status: i32, result: ^bool): void;
const register := fn(cb: Callback): void { ... };
```

## Pointers vs. References
- When possible, references should be used over pointers
- While references and pointers act similarly, references are guaranteed to be non-null
- Any pointer type can receive the builtin `nullptr` pointer
    - This is the only use-case for a built-in null in ghoti
- A reference to an object can access its internal fields using the dot operator
- You can dereference a pointer by using the `*` operator
- A pointer to an object can access its internal fields using the standard dot operator
    - Note that this is just syntactic sugar for an explicit dereference expression followed by a dot operator
```ghoti
var a: ^A = ...;
a.b;   // Syntactic sugar
(*a).b; // Equivalent expression
```
- You can get a pointer to an object using the `^` prefix operator
    - `^`: Returns a constant pointer to the object
    - `^mut`: Returns a constant pointer to the object
- You can get a reference to an object using the `&` prefix operator
    - `&`: Returns a constant reference to the object
    - `&mut`: Returns a constant reference to the object

## Type Introspection
- You can get the type of a value by using the `@typeOf` builtin
- You can get the size of a type (in bytes) by using the `@sizeOf` builtin
- You can get the align of a type by using the `@alignOf` builtin

## User-Defined Types
- User defined types (enums, unions, and structs) can be declared inline (as an explicit type) or out of line (as an expression)
- It may be helpful to return a user-defined type from a function (e.g. a generic type like an array list)
    - In this case, the return type should be of type `type`
- Generated types are done at compile time and functions which wish to return a type must have compile-time known parameters like types or integral constants
```ghoti
const Hand := union {
    card: bool,
    pen: i32,
    shoe: void,
    deck: struct {
        var hearts: [12uz]Card;
        var diamonds: [12uz]Card;
        var clubs: [12uz]Card;
        var spades: [12uz]Card;
        var has_jokers: bool;
    },
};
```

## Primitive Types
- `i32`: A 32-bit signed integer, represented by a standalone number
- `i64`: A 64-bit signed integer, represented by a `l` suffixed number
- `isize`: A platform specific signed integer equivalent to ptrdiff_t (64-bit on 64-bit systems), represented by a `z` suffixed number
- `u32`: A 32-bit unsigned integer, represented by a `u` suffixed number
- `u64`: A 64-bit unsigned integer, represented by a `ul` suffixed number
- `usize`: A platform specific unsigned integer equivalent to size_t (64-bit on 64-bit systems), represented by a `uz` suffixed number
- `f32`: A 32-bit floating point, represented by a `f` suffixed number (base-10), decimal, or scientific notation
- `f64`: A 64-bit floating point, represented by a standalone decimal or scientific notation
- `u8`: An unsigned 8-bit value, typically used for characters (i.e. `'a'`) or ascii values
    - There is no string type in ghoti. Instead you must use an array or slice of bytes (i.e. `[]u8` or `[N]u8`)
- `bool`: True (`true`) or false (`false`)
- `void`: The unit type, this can be declared using empty curly braces `{}`

## Special Types
- `type`: A type encapsulating the information presented in this documentation
    - Useful for generic programming where a struct is returned from a function to create a custom container or type
- `auto`: This type is used to tell function signatures that the type should be deduced by the call arguments
    - This is restricted to values and cannot be used to represent a type, see the `type` type
    - This is valid for parameters and for the return type
    - It can also be used in declarations, but this behaves the same as the walrus (`:=`) inference operator
- `opaque`: This type indicates a type that is not known to the compiler for some reason
    - Opaque types must be behind a pointer and cannot be dereferenced (mutability is allowed, but references are not)
    - This exists to fulfill the need introduced by C's `void*` idiom
- `noreturn`: Used to indicate that a function will never return to the caller
    - This is only valid on functions and does not support type modifiers
