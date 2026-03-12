# Loops
- There are 4 different loops in conch

## Do-while Loops
- Equivalent to the C/C++ do-while
- The loop's body is guaranteed to be evaluated at least once before the condition is tested
- The loop's body cannot be empty
```conch
do { a; } while (b != c);
```

## For Loops
- For loops are explicitly designed for array/slice iteration
- For loops have 4 different clauses:
    - Iterables: A comma separated list in parenthesis. This is the first clause and is required
    - Captures: A comma separated list enclosed by bitwise or `|` pipes. The is the second clause and is required. If you would like to discard a capture, use `_`
        - There must be the same number of captures as there are iterables
    - Body: The loop body, which encloses all values from the outer scope and captures
    - Non-break: An `else` clause that is located after the loop that only runs if the loop reached the end of execution naturally (i.e. there was not a break statement)
- The loop's body cannot be empty
```conch
for (arr, l, p) |i, &mut j, _| { ... }; // Captures can have modifiers or be discarded
for (0..4) |i| { ... } else return b; // The return statement only runs if the loop body does not break
for (0..4) |i, j| { ... } else { return b; } // Illegal, the number of captures and iterables must match
```

## Infinite Loops
- The equivalent to a `while (true)` loop in C
- These loops are never intended to end naturally, requiring an explicit control flow statement to exit execution
- The loop's body cannot be empty
```conch
loop { a; };
```

## While Loops
- While loops have 4 different clauses:
    - Condition: An expression enclosed by parenthesis. This is the first clause and is required
    - Continuation: An expression preceded by a colon, enclosed by parenthesis. This is the second clause and is purely optional
        - This expression is evaluated in between loop iterations, meaning it will not run before the first iteration nor after the last iteration or after a break.
    - Body: The loop body, which encloses all values from the outer scope
    - Non-break: An `else` clause that is located after the loop that only runs if the loop reached the end of execution naturally (i.e. there was not a break statement)
- The loop's body cannot be empty unless there is a continuation clause
```conch
while (true) : (i += 1) {a;}; // (i += 1) is evaluated evaluated in between loop iterations
while (true) {a;} else return b; // The return statement only runs if the loop body does not break
```

## Important Considerations
- The non break-clause of `for` and `while` loops are restricted statements, meaning they can only be:
    - Jump statements (return, break, continue)
    - Expressions
    - Blocks
- Loops are expressions and must be terminated by a semicolon
