# Match
- In addition to `if` control flow, conch has match expressions
- Match expressions can easily switch over enum values, but are also helpful when working with unions
- When matching a union, you must use a capture group with each match arm
    - This will capture the active field value of the union
    - This can also be discarded using an underscore `_`
    - There is no concept of a modifier with this capture, with access modifiers being dependent on the match object/the union's field types
- Match expressions must always be exhaustive
    - To catch all arms that were not explicitly listed, you can use a catch-all clause
    - Similar to `for` and `while` non-break, the catch-all clause is a restricted statement, meaning it can only be a:
    - Jump statement (return, break, continue)
    - Expression statement
    - Block statement
- Loops are expressions and must be terminated by a semicolon
```conch
match (a) { 2 => { c; } } else d; // Standard match on a non-union value
match (a) { .b => |b| { c; } }; // Match on a union value that has a field `b`
```
