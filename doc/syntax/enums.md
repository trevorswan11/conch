# Enums
- Enums are extremely simple in Conch, consisting of:
    - An optional backing integral type
    - Variants
    - Optional default values for variants
- Enums are strongly typed, and are declared with the standard declaration syntax
    - As enums are types, they can also be defined with a `using` statement. This restricts the modifiers you can use, preventing access/abi restrictions
- In the event that the standard declaration syntax is used, it must be `const`
- The backing type of an enum is defaulted to the smallest available type, but can be set to any integer (signed, unsigned, or byte)
- Enums can be casted to and from their underlying type by using the `@cast` builtin
```conch
const Colors := enum { RED, BLUE = 3, GREEN };
const Shapes := enum : ulong { CIRCLE, SQUARE };
```
- Enum variants are namespaced, meaning they do not leak into their outer scope as they would in C
- To access an enum's variant, the `::` operator is used
```conch
Colors::RED;
```
- You can not put methods on an enum as you would with a struct
