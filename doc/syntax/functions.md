# Functions
## Top-Level Function Declarations
- Functions are defined using the standard declaration syntax
- Function parameters are immutable by default
```conch
const foo := fn(a: int, b: uint): ulong {
    // ...
};
```

- A parameter may be made mutable by marking it as `ref`
    - To call such a function, the call site must also indicate `ref`
```conch
const foo := fn(ref a: int): ulong {
    // ...
};

var a := 2;
const out: ulong = foo(ref a);
```

- 'Mutable' functions can mutate the state of outer variables and are denoted with `mut`
- A functions mutability is tied to its type, meaning potential reassignments must match mutability
```conch
const bar := mut fn(): ulong {
    // ...
};
```

## Local Function Declarations
- Local functions (closures) are defined using the standard declaration syntax
- Local functions capture variables implicitly, only using what they need
- A local function must be marked `mut` to modify external variables
```conch
const foo := fn(a: int, b: uint): ulong {
    var something := 2;
    // Functions can be used as types, they're first class citizens!
    var bar: fn(c: ulong): void = fn(c: ulong): void {
        // Local functions implicitly capture necessary variables from the outer scope
        const d := (1 + b) / a; 
               
        // Compile Error - Illegal as enclosing function is not marked 'mut'
        // something += 2;
        // ...
    };

    // Local functions can be marked var and can be reassigned to a function of the same type
    bar = ...;
};
```

- There is no function overloading
- Top-level functions cannot be marked variable and must be `const`
    - This is because of how top-level functions behave with the resulting assembly
