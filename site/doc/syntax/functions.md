# Functions
- Functions are defined using the standard declaration syntax
- Function types can be aliased with a `using` statement
- Function parameters are immutable by default
```ghoti
const foo := fn(a: i32, b: u32): u64 {
    // ...
};
```

## Parameters
- Parameters obey the same syntax rules as the rest of the type system
- Without any type modifiers, parameters are passed by value
    - All parameters are implicitly constant values
- A parameter may be passed by const reference by marking it as `&`
    - To call such a function, the call site must also indicate with the `&` operator
    - A const pointer is created with the `^` operator which the function must also indicate
- A parameter may be passed by mutable reference by marking it as `&mut`
    - To call such a function, the call site must also indicate with the `&mut` operator
    - A mutable pointer is created with the `^mut` operator which the function must also indicate
- There are no default parameters
```ghoti
const foo := fn(a: &i32): u64 {
    // ...
};

const bar := fn(b: i32): u64 {
    // ...
};

const baz := fn(c: &mut i32): u64 {
    // ...
};

var a := 2;
_ = foo(&a);        // Allowed, passed by const reference
_ = bar(a);         // Allowed, passed by value
_ = baz(&mut a);    // Allowed, passed by mutable reference

const b := 1;
_ = foo(&b);        // Allowed, passed by const reference
_ = bar(b);         // Allowed, passed by value
_ = baz(&mut b);    // Illegal, cannot mutate const
```

- Variadic function parameters are supported to allow for c-interop
    - Currently, ghoti does not expose primitives for interacting with this parameter type
    - For example, you can declare an extern function like `printf` from libc as: `extern const printf: fn(^u8, ...): i32;`

## Semantics
- There is no function overloading
    - This includes operators. There is no operator overloading
- Top-level function declarations cannot be marked variable and must be `const`
- Functions can have the `noreturn` return 'type' which signifies that the compiler should not expect a `return` construct in the function body
    - Violating this assumption is a compile time error
- The return types `noreturn` and `void` cannot have any type modifiers
- You are able to declare local functions (i.e. functions inside of functions) or anonymous functions (i.e. functions passed directly as a parameter to a function)
- Closures are implicit as they are in rust
    - A closure implicitly captures referenced values by mutable reference if the value is used in mutable contexts
    - Non-value objects that can be trivially copied and are not mutated in the closure are copied
- The type of a closure is not known to the programmer

## Builtin Functions
- Builtin functions are prefixed with the `@` symbol and are always camelCase
- Builtin functions can run at compile time or runtime depending on the context, and will attempt to lower to the most efficient instructions when possible

## Calling
- Functions are called in three ways depending on the context
    - If calling a top-level, non-struct function, the syntax is simple (`func(args...)`)
    - If calling a member function, the syntax is similar to other languages (`instance.func(args...)`)
        - This requires the function to have a self parameter, as discussed in the struct, enum, and union documentation
        - The callee must have mutability that matches the explicit self parameter
    - If calling a 'static' struct function (one without a self parameter), the syntax uses the dot operator (`Struct.func(args...)`)

## Types
- Function's signatures are their types, including parameter modifiers and the return type
    - Note that these signatures do not contain parameter names (this is not supported, even optionally)
    - Signatures do include the variadic argument distinction
- A function can be declared verbosely by using the type before the declaration function
```ghoti
const f: fn(bool): i32 = fn(b: bool): i32 { ... };
```

- You can also use this to indicate that a function takes a function as an argument
```ghoti
const f := fn(g: fn(): i32): i32 { ... };
```

## Constexpr functions
- Functions declared and assigned to `constexpr` declarations signal to the compiler that the function may be called at compile time
    - 'May' meaning that it will be called at compile time at a `constexpr` context
- There are restrictions imposed on `constexpr` functions that do not apply to normal ones
    - IO must not be performed (reading from/writing to disk, via threading, printing to stdout, etc)
    - Heap allocations may not be made
    - TODO
- A local function can be declared `constexpr` only if it captures no values
