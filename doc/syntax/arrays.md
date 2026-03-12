# Arrays
## Declarations
- Arrays are defined with 3 key components
    - A size: This must be an underscore (signifying inferred size), a compile time constant (denoted with `comptime`), or a usize integer literal (suffix `uz`)
    - A type: This can be any valid type, but it must be present and cannot be inferred
    - Items: These items must be of the type specified, and must match the provided size if provided
- Arrays are immutably sized, though internal elements can be mutable based on the provided type
- A trailing comma is allowed, but not required
```conch
[_]*N{a, b, c, d, e, }; // Inferred Size
[2uz]int{A, B, }; // Explicit size
[1uz]int{2, 3}; // Illegal - size mismatch
```

- Arrays do not decay to pointers implicitly as they do in C
- To call a C function that takes in a pointer that is actually an array, you can use the builtin `@ptrFromArray`
    - This erases the underlying size of the array and is not reversible. It is only recommended for use with C interop
- If you have a pointer and its size, you can recreate a conch-style slice using the `@sliceFromPtr` builtin

## Types
- There are two types of Array types 
    - Array types are extremely similar to their declaration counterpart, requiring two key components
        - A size: This must be a compile time constant or a usize integer literal
        - A type: This can be any valid type, but it must be present and cannot be inferred. It can be as deeply nested as you'd like (i.e. `[2uz][2uz]int`)
    - Slice types differ from array types in that they lack a size. They are composed of:
        - An open-close bracket pair with nothing in between
        - A type: exactly the same as an array
    - Slices do not have to be of fixed size as they are technically a fat pointer. They store both a pointer and size
    - Values of type 'array' have an implicit `size` field that can be accessed used the dot operator
    - Values of type 'slice' also have this `size` field, but are equipped with a second field called `ptr` to access the pointer directly
    - Values of either type can be used with array-related builtins
- Array types can have modifiers
```conch
var a: &[S]&*mut T; // Normal array type
var a: &[]&*mut T; // Analogous slice type
```
- Arrays do not implicitly 'decay' to slices
    - To pass an array to a function that expects a slice, you should use the respective reference operator (`&` or `&mut`) depending on the context. This is similar to Zig's array/slice syntax

## Indexing
- Array indexing is bounds-checked, and follows a familiar syntax
- Array indices can take two forms:
    - A single value: `arr[i]`
    - A range of values (lower bound is always inclusive):
        - Exclusive range: `arr[i..j]`
        - Inclusive range: `arr[i..=j]`
- Indexing an array with a single value returns a value that is of the same type as the array's inner items
    - This can be used to chain array indexing as: `const val := matrix[0uz][1uz];`
- Indexing an array with a range returns a slice that is exactly the size specified by said range
- An attempt to index outside of the bounds of an array results in a crash
- If an array has decayed to a pointer, there are a few ways to interact with the memory through builtins:
    - `@ptrAdd(ptr, offset)`: Returns a pointer to the value at `ptr + offset`
    - `@ptrSub(ptr, offset)`: Returns a pointer to the value at `ptr - offset`
    - `@ptrIdx(ptr, offset)`: Returns a pointer to the value at `offset`
- It is not necessary to scale the offset by the size of the underlying item type
    - This is a footgun from C that is handled internally by conch
- These builtins are `not` safety checked, and illegal access has undefined behavior
    - They should be avoided unless strictly necessary (i.e. working with highly performance-critical code or C-interop)

## Memory layout
### Arrays
- Arrays are guaranteed to be contiguous in memory
- Arrays do not support implicit sentinel termination, the size provided to an array at initialization is exactly the size of that array during use

### Slices
- Slices are represented by the equivalent size of two `usize` members (e.g. 16 bytes on a 64-bit system)

## Iteration
- Arrays/slices can be iterated over in two ways:
    - Looping through all possible index values (i.e. manually walking an index from low -> high)
    - Looping through the array/slice directly
- An array/slice can be directly iterated over such that items are constant in the loop body:
```conch
const a := [_]int{1, 2, 3, 4};
for (a) |b| { ... };
```
- An array/slice can be mutable iterated over if the array/slice is mutable and the underlying type is also mutable (i.e. not a constant pointer/reference)
- Arrays that hold value types inherit the mutability of the array itself when being iterated
```conch
var a := [_]int{1, 2, 3, 4};
for (a) |&mut b| { ... };

const c := [_]int{1, 2, 3, 4};
for (c) |&mut b| { ... }; // Illegal, c is a constant
```
