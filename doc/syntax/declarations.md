# Declarations
- The assignment operator `=` can be used to assign an existing non-constant value
    - It is also used to assign a value to an explicitly typed variable in-line
    - The operator can also be used to discard a value using the `_ = ...` syntax
- You can discard expression results using the `_ = ...` syntax
    - In ghoti, identifiers cannot start with `_` and thus cannot be named `_`
- The walrus operator `:=` can be used to declare a value who's type is inferred based on the assigned value
    - The walrus operator can only be used with value-initialized declarations
- There is no shadowing in ghoti
    - You cannot declare the same variable name in the same scope or enclosing scope

```ghoti
const a := 2;           // Type deduced to be i32
const b := "str";       // Type deduced to be a constant size array of bytes (non-null terminated)
const c: byte = 's';    // Explicitly typed, so value must agree
const d :=;             // Illegal, walrus needs a value!
var e: []byte;         // Allowed, e is forward declared and future assignments must be a slice of bytes
```

## Runtime Constants
- Constants are declared with the `const` keyword
- A constant must be initialized with a value
    - This is true unless it is a value that is marked `extern`
- A constant's value does not have to be compile-time known
- Constness cannot be casted away
- A mutable reference cannot be taken from a constant

## Compile-time Constants
- Compile-time-known constants are declared with the `constexpr` keyword
- In most cases, `constexpr` values behave identically to `const` constants with a few key differences
    - `constexpr` values must be computable at compile time
    - `constexpr` functions must be callable at compile time (it is safe to call them at runtime as well)
- A `constexpr` declaration can not be declared `extern`

## Variables
- Variables are declared with the `var` keyword
- A variable can be initialized with or without a value
    - Variables without an initial value must be initialized with an explicit type
    - Variables declared without an initial value are necessarily uninitialized, and reading from them in this state is illegal behavior
    - Variables can be explicitly marked as uninitialized with the `undefined` keyword. Using this as the right hand side of a declaration is equivalent to not using a value at all
- A variable's value does not have to be compile-time known
- A variable may be assigned to a constant

```ghoti
const a := 1;       // Allowed, see above
var b := a;         // Allowed, a is copied by value
var c := &mut a;    // Illegal, cannot take mutable reference of constant
```

## Modifiers
- Declarations have many different modifiers that affect linkage, access, and ownership
    - `pub`: When used inside of a struct, this opens access to non-struct-local functions (i.e. member functions). When used on a top-level declaration, this lets importing files access the declaration. The presence of this keyword correctly implies that all declarations are *private* unless explicitly stated otherwise.
    - `extern`: Denotes a declaration as relating to a symbol yet-to-be defined (i.e. external linkage). This currently supports only C symbols. This keyword cannot be combined with the `export` modifier. 
    - `export`: Forwards the declaration to the 'outside world'. This means that the symbol is treated as a C symbol. This keyword cannot be combined with the `extern` modifier.

```ghoti
pub var c := 2;          // Allowed, symbol can be imported
extern const a: i32;     // Allowed, externs must be explicitly typed without values
export var b := 1;       // Allowed
extern constexpr a: i32; // Illegal, inherently contradictory
```

## Assignment
- Non-const declarations can be reassigned
- Re-assignment is right associative, meaning that you can chain assignment expressions:
```ghoti
var a: i32 = 2;
var b: i32 = 5;
a = (b = 6);
```
- This works because an assignment returns the assigned value

## Type Aliasing
- Type aliases can be declared with the `using` keyword
- An alias can be marked public in order to expose it to the outside world when the enclosing file is imported
- This is very similar to C++, for example:
```ghoti
using MyBool = bool; // Private to this file
pub using MyPublicBool = bool; // Publically accessible via namespacig when imported
```
- Note that this is an _alias_, so there is no difference between using the aliased type and handwriting the entire raw type
- The most common use cases for this are for saving keystrokes for long types and for passing complex types to functions at the call site
    - Since function calls prioritize expressions over types, so an attempt to pass `&mut P` will always be parsed as a mutable reference to an object P
    - Declaring the type using `using T = &mut P;` allows you to then pass the identifier to the function
    - This is more explicit than having to go to the function definition to see what is being expected at the call site
- Note that these statements are private by default and can be prefixed with `pub` to unrestrict visibility
- You may declare types such as unions, enums, and structs with this declaration, though they must be bare types without modifiers
    - This restriction is in place as it would be impossible to instantiate the underlying type
```ghoti
using S = &struct { ... };          // Illegal
using S = *union { ... };           // Illegal
using S = volatile enum { ... };    // Illegal
using S = enum { ... };             // Legal
```
