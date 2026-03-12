# Control Flow
## Jumps
- Jump statements are implemented using the `return`, `break` and `continue` keywords
- The `return` keyword is the only jump statement that allows an associated value
    - This value can be any expression, including a type alias or declaration (new union, struct, or enum)
- `continue` and `break` can only be used in loop bodies, while `return` can only be used inside of functions
- `continue` and `break` will always apply to the immediately enclosing loop

## Conditional Expressions
- If-else expressions are a crucial part of control flow
- They are composed of a required conditional and consequence with an optional alternate
- The alternate clause is a restricted else statement, similar to `match`, `for`, and `while` loops. Similarly, it can only be a:
    - Jump statement
    - Expression statement
    - Block statement
- If the consequence and alternate statements are both expression statements, then the resulting value is returned
```conch
a = if (a) b; else c; // Assigns a conditionally
```
