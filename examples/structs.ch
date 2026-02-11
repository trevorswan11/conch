// Standard structs
const Foo := struct {           // Standard declaration with type inference
    var bar: int;               // Mutable member variable
    const baz: string = "baz";  // Constant member variable, must be initialized in-line

    private var boo := 4u;      // Members are public by default
    static const foo := 3.4;    // Static variables are struct globals and are not instance specific

    const worker_one := fn(): void {                // Functions have an implicit 'this' parameter, unless marked static
        // ...
    };

    const worker_two := mut fn(): void {            // Member functions marked 'mut' can mutate state
        // ...
    };

    static const worker_three = fn(): void {        // Functions have an implicit 'this' parameter, unless marked static
        // ...
    };

    private const fee := "Hello, World!";           // Members can be placed anywhere in the struct definition
};

// Packed structs restrict the compiler from reordering members or adding padding to members
const Bar := packed struct {
    // ...
};
