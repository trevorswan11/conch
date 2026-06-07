# Unions
- Unions in ghoti act very similar to tagged unions in other languages
- Unions are defined using the standard declaration syntax or with a `using` statement
    - In the event that the standard declaration syntax is used, it must be `const`
- A union is defined by a a comma separated list of key-value pairs, where:
    - Each key is an identifier which is used for the active tag
    - Each value is a type for said identifier
- Only one field of a union can be active at one time
- Attempting to access an inactive field of a union is safety-checked
- A union field's identifier can be used to check if it is active by using the equality operator and implicit access expression
    - You can also use this to match over an union, capturing the active fields value with a capture clause
- The inner types of a union are not restricted to any subset or superset of the language (i.e. you are not restricted to only primitive types nor are you prohibited from using user-defined types)
- Initializing a union is done through the exact same mechanism as structs, using the `Name{.field = value}` syntax (shorthand `.{.field = value}`)
- To access a union's members, the `.` operator is used
```ghoti
const Hand := union {
    card: bool,
    pen: i32,
    shoe: void,
};

var h1 := Hand{ .card = true }; // Initializes h1 to have the card field active
var h2: Hand = .{ .card = true }; // Equivalent to h1

if (h1 == Hand.card) { ... }; // Checks if the card field is active
if (h1 == .card) { ... }; // Equivalent to above

match (h1) {
    .card => |c| { ... } // Capture the boolean field if active
    .pen => |_| { ... } // Discarded
    .shoe => { ... } // No capture
};
```
- You can put declarations inside of an union as you would with a struct
    - Member functions have a first argument which is an instance of the enum as explained in the struct documentation
    - There is no concept of an instance field in an enum
    - These members must appear after all fields
- Union 'members' can also be `using` or `import` statements
