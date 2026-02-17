# Structs
- Structs are defined using the standard declaration syntax
- Struct definitions must be `const`
- Struct members are simply declarations
    - Members cannot be marked `extern` or `export`
    - Members can be marked `private` to prevent external access
    - Members can be marked `static`, denoting struct-level ownership (as opposed to instance)
    - Functions that wish to mutate state (either statically or not) must be marked as such (`mut`)
    - Functions are considered to be top-level within the struct and must be `const`
    - Members can be declared in any order, the compiler is order independent and is free to reorder to optimize
- Struct types are internal to the compiler and should never be written by hand (compile error)
    - Struct definitions must use the walrus operator `:=`
    - The type of a struct can be retrieving by using the `@typeOf` builtin
- Static members are resolved using the `::` operator
- Instance members are resolved using the `.` operator

```conch
const Foo := struct {           // Standard declaration with type inference
    var bar: int;               // Mutable member variable
    const baz: string = "baz";  // Constant member variable, must be initialized in-line

    private var boo := 4u;      // Members are public by default
    static const foo := 3.4;    // Static variables are struct globals and are not instance specific

    const worker_one := fn(): void {                // Functions have an implicit 'this' parameter, unless marked static
        // ...                                      // Member functions must be const
    };

    const worker_two := mut fn(): void {            // Member functions marked 'mut' can mutate state
        // ...
    };

    static const worker_three = fn(): void {        // Functions have an implicit 'this' parameter, unless marked static
        // ...
    };

    static const worker_three = mut fn(): void {    // Mutable static functions can mutate variable static members
        // ...
    };

    // Compile Error - top-level functions cannot be marked 'var'
    // var worker_four := fn(a: int, b: uint): ulong {
    //
    // };

    private const fee := "Hello, World!";           // Members can be placed anywhere in the struct definition
};

Foo::foo;                                           // Static members are resolved using the '::' operator
Foo::worker_three();                                // The same goes for functions
```

- Structs can be marked `packed` to prevent the compiler from reordering members or from adding additional padding

```conch
const Bar := packed struct {
    // ...
};
```

- There is no inheritance
- Interfaces are not natively supported
