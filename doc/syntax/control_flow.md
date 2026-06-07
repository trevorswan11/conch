# Control Flow
## Entry Point
- The entry point of a program must be a public `main` function that returns an integer or void
    - The function may return an integer (`i32`) when a non-zero exit code may be desired
    - Using `void` is equivalent to the implicit return behavior observed in C
- This function must take in a single parameter of type `[][:0]u8` representing the passed command line args
    - This is conventionally named `args` but can be named to your choosing
    - Note that these args are null terminated to respect C-interop with minimal friction
```ghoti
import std;

pub const main := fn(args: [][:0]u8): void {
    const message := "Hello, world!";
    std::io::println(message);
};
```

## Labels
- Labels can be applied to the following statements and expressions for expressiveness:
    - All loops
    - If expressions
    - Match expressions
    - Blocks
- Labels are declared as a single identifier followed by a colon before the construct is reached (e.g. `blk: {}`)
- Inside of a block, you can break with a value
    - If there is no value but you break out of a block, the resulting block is of `void` type
- When labels are applied to loops, you can control which iteration is skipped or broken from
    - This is particularly useful in cases of nested loops

## Jumps
- Jump statements are implemented using the `return`, `break` and `continue` keywords
    - `return` and `break` statements are the only constructs that can return a value upon execution
- The `return` keyword is the only jump statement that allows an associated value
    - This value can be any expression, including a type alias or declaration (new union, struct, or enum)
- `break` can be used in loop or label bodies
- `continue` in loop bodies
    - These statements cannot have an associated value in any case
- `return` can only be used inside of functions
- `continue` and `break` will always apply to the immediately enclosing loop without a label
    - Without a label, a value cannot be specified with the statement

## Conditional Expressions
- If-else expressions are a crucial part of control flow
- They are composed of a required conditional and consequence with an optional alternate
- The alternate clause is a restricted else statement, similar to `match`, `for`, and `while` loops. Similarly, it can only be a:
    - Jump statement
    - Expression statement
    - Block statement
- If the consequence and alternate statements are both expression statements, then the resulting value is returned
```ghoti
a = if (a) b; else c; // Assigns a conditionally
```
- Conditionals can be evaluated at compile time with a syntax similar to C++
```ghoti
if constexpr (true) {
    ...
} else {
    ...
}
```
- In the above example, the top block is the only statement evaluated, with the `else` branch and condition being completely dropped at runtime
- Statements inside the consequent of a conditional are still scoped as written, `constexpr` will not hoist a branch into the outer scope

## Defer
- Defer statements defer execution to the end of their scope
- They are executed in reverse order with respect to their declaration
- A defer statement can be an expression, block, or discard statement
    - All other inner statement variants are disallowed
```ghoti
import std;

const main := fn(): void {
    defer std::io::println("World!");
    defer std::io::print("Hello, ");
};
```
- This example prints "Hello, World!"
    - Every defer statement pushes execution onto a LIFO stack that is executed at the end of scope
