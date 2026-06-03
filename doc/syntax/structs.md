# Structs
- Structs are defined using the standard declaration syntax or with a `using` statement
    - In the event that the standard declaration syntax is used, it must be `const`
- Struct definitions must be `const`
- Structs may have a comma-separated list of fields that are owned by instances of the struct
    - These must be the first thing to appear in the struct
    - They are declared with an optional `pub` to mark access modifiers, followed by the name, type, and optional default value: e.g. `pub foo: i32 = 23`
    - Members do not have an individual idea of mutability, simply inheriting the mutability of their enclosing object when used
        - This is of course only applied to value types as pointers and references enforce local immutability
- Structs can also have member declarations
    - Once the first declaration is hit, there may not be any more instance members
    - Decls here cannot be marked `extern` or `export`
    - Decls here can be marked `pub` to allow external access
    - Member functions have a first argument which is an instance of the struct: `self`
        - A member function can provide this keyword in five different ways:
            1. `self` denotes a pass by value (copy)
            2. `&self` denotes a pass by const reference
            3. `*self` denotes a pass by const pointer
            4. `&mut self` denotes a pass by mutable reference
            5. `*mut self` denotes a pass by mutable pointer
        - This parameter _must_ be the first parameter of the function's parameter list
        - This parameter is conventionally named `self` but is allowed to assume any non-reserved keyword
        - This parameter has the underlying type of the directly enclosing struct
    - Declared functions without a self parameter can be thought of as `static` in other languages
    - Functions are considered to be top-level within the struct and must be `const`
    - Declarations must always come after a list of instance members (which may be empty)
- Struct 'declarations' can also be `using` or `import` statements
- All internal accesses are resolved using the `.` operator

```ghoti
const Foo := struct {           // Standard declaration with type inference
    bar: i32,                   // Private field without default value (must appear in initializer list)
    pub boo: u32 = 4u,          // Public field with default value

    pub var foo := 3.4;         // Decl variables are struct globals and are not instance specific
    const baz: []byte = "baz";  // Constant decl, must be initialized in-line

    const worker_one := fn(&self): void {           // Functions can have an explicit 'self' parameter
        // ...
    };

    const worker_two := fn(&mut self): void {       // The self parameter can be marked '&mut' to mutate state
        // ...
    };

    pub const worker_three = fn(): void {        // Functions without 'self' parameter cannot take an instance of the struct
        // ...
    };

    // Compile Error - top-level functions cannot be marked 'var'
    // var worker_four := fn(a: i32, b: u32): u64 {
    //
    // };

    pub const fee := "Hello, World!";           // Decls can be placed anywhere after the fields
};

Foo.foo;                                           // All members are resolved using the '.' operator
Foo.worker_three();                                // The same goes for functions

var foo: Foo = .{ .bar = 42, };
Foo.worker_two(&mut foo);   // Member functions may be called explicitly 
foo.worker_two();           // Or with this shorthand
```

- There is no inheritance
- Interfaces are not natively supported

## Initialization
- Structs can be initialized in two ways
    - You may use the structs name followed by a braced list of implicit access declarations assigned to values
    - You may use the `.{}` syntax followed by the same list as above
- The below example demonstrates both of these mechanisms
```ghoti
const S = struct { var a: int; };

const s1 := S{ .a = 3 };
const s2: S = .{ .a = 3 };
```
- All member variables that do not have default values associated with their declarations must be initialized in either case
