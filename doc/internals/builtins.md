# Builtin Functions
Ghoti has the following builtin functions that are provided by the runtime and can be called form any scope. They are typically functions that facilitate low-level control and often make use of dedicated hardware instructions when available. This list is exhaustive though proposals for modifications, additions, and deletions are welcome.

## Casting

### @alignCast
```ghoti
@alignCast(T: type, expression: auto): T
```

Casts the alignment of the expression's pointer value to that of the provided type. Both `T` and `expression` must be pointer types.

### @ptrCast
```ghoti
@ptrCast(T: type, expression: auto): T
```

Casts the pointer result of `expression` to pointer `T`. Both `T` and `expression` must be pointer types.

### @bitCast
```ghoti
@bitCast(T: type, expression: auto): T
```

Converts a value of one type to another provided type.

### @constCast
```ghoti
@constCast(expression: auto): auto
```

Casts away top-level `const`-ness of the expression, preserving all other aspects of the type.

### @volatileCast
```ghoti
@volatileCast(expression: auto): auto
```

Casts away top-level `volatile`-ness of the expression, preserving all other aspects of the type.

### @as
```ghoti
@as(T: type, expression: auto): T
```

Coerces the value of `expression` into a value of type `T`.

## Pointer Operations

### @intFromPtr
```ghoti
@intFromPtr(expression: auto): usize
```

Returns the memory address as a `usize` integer.

### @ptrFromInt
```ghoti
@ptrFromInt(T: type, expression: usize): T
```

Returns a pointer type `T` at the memory address represented by the `usize` integer. `T` must be a pointer type.

### @ptrFromArray
```ghoti
@ptrFromArray(expression: auto): auto
```

Returns a pointer to the first element of the array-yielding expression. 

### @sliceFromPtr
```ghoti
@sliceFromPtr(expression: auto, len: usize): auto
```

Constructs a slice from the pointer-yielding expression and length.

## Introspection & Metadata

### @alignOf
```ghoti
@alignOf(expression: auto): usize
```

Computes the alignment of the provided expression in bytes.

### @sizeOf
```ghoti
@sizeOf(expression: auto): usize
```

Computes the size of the provided expression in bytes.

### @typeOf
```ghoti
@typeOf(expression: auto): type
```

Returns the type of the provided expression.

### @this
```ghoti
@this(): type
```

Returns the type of the current enclosing structural type (i.e. struct, enum, union). Invalid in all other contexts.

### @tagName
```ghoti
@tagName(expression: auto): [:0]u8
```

Returns a null-terminated static string representing the name of the enum or active union tag yielded by the expression.

## Memory Operations

### @memcpy
```ghoti
@memcpy(src: auto, dest: auto): void
```

Copies all bytes from src to dest. Requires the two memory regions to be unique.

### @memset
```ghoti
@memset(mem: auto, expression: auto): void
```

Sets all values in the array or slice to the expression.

### @memmove
```ghoti
@memmove(src: auto, dest: auto): void
```

Moves all bytes from src to dest. Allows the two memory regions to overlap.

## Math

### @mulAdd
```ghoti
@mulAdd(T: type, a: T, b: T, c: T): T
```

Fused multiply-add, similar to (a * b) + c, except only rounds once, and is thus more accurate. T must be a floating point type.

### @clz
```ghoti
@clz(expression: auto): usize
```

Count the number of leading zeros in the expression's bit representation.

### @ctz
```ghoti
@ctz(expression: auto): usize
```

Count the number of trailing zeros in the expression's bit representation.

### @popCount
```ghoti
@popCount(expression: auto): usize
```

Computes the number of set bits in the expression's binary representation.

### @sqrt
```ghoti
@sqrt(expression: auto): @typeOf(expression)
```

Computes the square root of the passed expression.

### @sin
```ghoti
@sin(expression: auto): @typeOf(expression)
```

Computes the sine of the passed expression.

### @cos
```ghoti
@cos(expression: auto): @typeOf(expression)
```

Computes the cosine of the passed expression.

### @tan
```ghoti
@tan(expression: auto): @typeOf(expression)
```

Computes the tangent of the passed expression.

### @exp
```ghoti
@exp(expression: auto): @typeOf(expression)
```

Computes the exponentiation `e^x` of the passed expression.

### @exp2
```ghoti
@exp2(expression: auto): @typeOf(expression)
```

Computes the exponentiation `2^x` of the passed expression.

### @log
```ghoti
@log(expression: auto): @typeOf(expression)
```

Computes the natural logarithm of the passed expression.

### @log2
```ghoti
@log2(expression: auto): @typeOf(expression)
```

Computes the base-2 logarithm of the passed expression.

### @log10
```ghoti
@log10(expression: auto): @typeOf(expression)
```

Computes the base-10 logarithm of the passed expression.

### @abs
```ghoti
@abs(expression: auto): @typeOf(expression)
```

Computes the absolute value of the passed expression.

### @floor
```ghoti
@floor(expression: auto): @typeOf(expression)
```

Computes the floor'ed value of the passed expression.

### @ceil
```ghoti
@ceil(expression: auto): @typeOf(expression)
```

Computes the ceil'ed value of the passed expression.

## Control Flow

### @panic
```ghoti
@panic(message: [:0]u8): noreturn
```

Immediately terminates execution and panics with the provided message.
