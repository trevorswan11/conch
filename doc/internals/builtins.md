# Builtin Functions
Porpoise has the following builtin functions that are provided by the runtime and can be called form any scope. They are typically functions that facilitate low-level control and often make use of dedicated hardware instructions when available. This list is exhaustive though proposals for modifications, additions, and deletions are welcome.

## Casting

### @alignCast
```porpoise
@alignCast(T: type, expression: auto): T
```

Casts the alignment of the expression's pointer value to that of the provided type. Both `T` and `expression` must be pointer types.

### @ptrCast
```porpoise
@ptrCast(T: type, expression: auto): T
```

Casts the pointer result of `expression` to pointer `T`. Both `T` and `expression` must be pointer types.

### @bitCast
```porpoise
@bitCast(T: type, expression: auto): T
```

Converts a value of one type to another provided type.

### @constCast
```porpoise
@constCast(expression: auto): auto
```

Casts away top-level `const`-ness of the expression, preserving all other aspects of the type.

### @volatileCast
```porpoise
@volatileCast(expression: auto): auto
```

Casts away top-level `volatile`-ness of the expression, preserving all other aspects of the type.

### @as
```porpoise
@as(T: type, expression: auto): T
```

Coerces the value of `expression` into a value of type `T`.

## Pointer Operations

### @intFromPtr
```porpoise
@intFromPtr(expression: auto): usize
```

Returns the memory address as a `usize` integer.

### @ptrFromInt
```porpoise
@ptrFromInt(T: type, expression: usize): T
```

Returns a pointer type `T` at the memory address represented by the `usize` integer. `T` must be a pointer type.

### @ptrFromArray
```porpoise
@ptrFromArray(expression: auto): auto
```

Returns a pointer to the first element of the array-yielding expression. 

### @sliceFromPtr
```porpoise
@sliceFromPtr(expression: auto, len: usize): auto
```

Constructs a slice from the pointer-yielding expression and length.

## Introspection & Metadata

### @alignOf
```porpoise
@alignOf(expression: auto): usize
```

Computes the alignment of the provided expression in bytes.

### @sizeOf
```porpoise
@sizeOf(expression: auto): usize
```

Computes the size of the provided expression in bytes.

### @typeOf
```porpoise
@typeOf(expression: auto): type
```

Returns the type of the provided expression.

### @tagName
```porpoise
@tagName(expression: auto): [:0]u8
```

Returns a null-terminated static string representing the name of the enum or active union tag yielded by the expression.

## Memory Operations

### @memcpy
```porpoise
@memcpy(src: auto, dest: auto): void
```

Copies all bytes from src to dest. Requires the two memory regions to be unique.

### @memset
```porpoise
@memset(mem: auto, expression: auto): void
```

Sets all values in the array or slice to the expression.

### @memmove
```porpoise
@memmove(src: auto, dest: auto): void
```

Moves all bytes from src to dest. Allows the two memory regions to overlap.

## Math

### @mulAdd
```porpoise
@mulAdd(T: type, a: T, b: T, c: T): T
```

Fused multiply-add, similar to (a * b) + c, except only rounds once, and is thus more accurate. T must be a floating point type.

### @clz
```porpoise
@clz(expression: auto): usize
```

Count the number of leading zeros in the expression's bit representation.

### @ctz
```porpoise
@ctz(expression: auto): usize
```

Count the number of trailing zeros in the expression's bit representation.

### @popCount
```porpoise
@popCount(expression: auto): usize
```

Computes the number of set bits in the expression's binary representation.

### @sqrt
```porpoise
@sqrt(expression: auto): @typeOf(expression)
```

Computes the square root of the passed expression.

### @sin
```porpoise
@sin(expression: auto): @typeOf(expression)
```

Computes the sine of the passed expression.

### @cos
```porpoise
@cos(expression: auto): @typeOf(expression)
```

Computes the cosine of the passed expression.

### @tan
```porpoise
@tan(expression: auto): @typeOf(expression)
```

Computes the tangent of the passed expression.

### @exp
```porpoise
@exp(expression: auto): @typeOf(expression)
```

Computes the exponentiation `e^x` of the passed expression.

### @exp2
```porpoise
@exp2(expression: auto): @typeOf(expression)
```

Computes the exponentiation `2^x` of the passed expression.

### @log
```porpoise
@log(expression: auto): @typeOf(expression)
```

Computes the natural logarithm of the passed expression.

### @log2
```porpoise
@log2(expression: auto): @typeOf(expression)
```

Computes the base-2 logarithm of the passed expression.

### @log10
```porpoise
@log10(expression: auto): @typeOf(expression)
```

Computes the base-10 logarithm of the passed expression.

### @abs
```porpoise
@abs(expression: auto): @typeOf(expression)
```

Computes the absolute value of the passed expression.

### @floor
```porpoise
@floor(expression: auto): @typeOf(expression)
```

Computes the floor'ed value of the passed expression.

### @ceil
```porpoise
@ceil(expression: auto): @typeOf(expression)
```

Computes the ceil'ed value of the passed expression.

## Control Flow

### @panic
```porpoise
@panic(message: [:0]u8): noreturn
```

Immediately terminates execution and panics with the provided message.
