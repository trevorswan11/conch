# Declarations
- The assignment operator `=` can be used to assign an existing non-constant value
    - The operator can also be used to discard a value using the `_ = ...` syntax 
- The walrus operator `:=` can be used to declare a value who's type is inferred based on the assigned value
    - The walrus operator can only be used with value-initialized declarations

```conch
const a := 2;           // Type deduced to be a signed 32 bit int
const b := "str";       // Type deduced to be a string
const c: byte = 's';    // Explicitly typed, so value must agree
const d :=;             // Illegal, walrus needs a value!
var e: string;          // Allowed, e is forward declared and future assignments must be a string
```

## Constants
- Constants are declared with the `const` keyword
- A constant must be initialized with a value
    - This is true unless it is a value that is marked `extern`
- A constant's value does not have to be compile-time known
- Constness cannot be casted away (for now)
- A mutable reference cannot be taken from a constant

## Variables
- Variables are declared with the `var keyword
- A variable can be initialized with or without a value
    - Variables without an initial value must be initialized with an explicit type
    - Variables declared without an initial value are necessarily uninitialized, and reading from them in this state is undefined behavior
- A variable's value does not have to be compile-time known
- A variable may be assigned to a constant

```conch
const a := 1;       // Allowed, see above
var b := a;         // Allowed, a is copied by value
var c := &mut a;    // Illegal, cannot take mutable reference of constant
```

## Modifiers
- Declarations have many different modifiers that affect linkage, access, and ownership
    - `private`: When used inside of a struct, this restricts access to struct-local functions (i.e. member functions) only. When used on a top-level declaration, this prohibits importing files from accessing the declaration. The presence of this keyword correctly implies that all declarations are `public` unless explicitly stated otherwise. This keyword is not valid with the `extern` or `export` modifiers.
    - `extern`: Denotes a declaration as relating to a symbol yet-to-be defined (i.e. external linkage). This currently supports only C symbols. This keyword cannot be combined with the `export` modifier.
    - `export`: Forwards the declaration to the 'outside world'. This means that the symbol is treated as a C symbol. This keyword cannot be combined with the `extern` modifier.
    - `static`: This keyword is only valid for struct members. It denotes a symbol as being owned (namespaced) by the struct itself, not by instances of said struct.

```conch
private var c := 2;     // Allowed, symbol can no longer be imported or exported
extern const a: int;    // Allowed, externs must be explicitly typed without values
export var b := 1;      // Allowed
static var c := 33;     // Illegal, cannot use static on a non-struct member
```
