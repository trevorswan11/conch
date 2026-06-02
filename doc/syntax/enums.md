# Enums
- Enums are extremely simple in porpoise, consisting of:
    - An optional backing integral type
    - Variants
    - Optional default values for variants
    - A list of member declarations similar to structs
- Enums are strongly typed, and are declared with the standard declaration syntax or with a `using` statement
    - In the event that the standard declaration syntax is used, it must be `const`
- In the event that the standard declaration syntax is used, it must be `const`
- The backing type of an enum is defaulted to the smallest available type, but can be set to any integer (signed, unsigned, or byte)
- Enums can be casted to and from their underlying type by using the `@cast` builtin
- For an enum to accept values that are outside of the values of its internal enumerations, you must end the enumeration list with an underscore `_`
    - You cannot access these via the dot operator, and their value may only be retrieved via a cast as in C++
```porpoise
const Colors := enum { red, blue = 3, green };
const Shapes := enum : u64 { circle, square, _ };
```
- Enum variants are namespaced, meaning they do not leak into their outer scope as they would in C
- To access an enum's variants and members, the `.` operator is used
```porpoise
const a := Colors.red;
const a: Colors = .red; // Equivalent
```
- You can put declarations inside of an enum as you would with a struct
    - Member functions have a first argument which is an instance of the enum as explained in the struct documentation
    - There is no concept of an instance field in an enum
    - These members must appear after all enumeration values
- Enum 'members' can also be `using` or `import` statements
