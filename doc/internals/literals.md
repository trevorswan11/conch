# Literals
Ghoti has numeric and identifier literals that carry intrinsic type information with their usage. 

## Identifiers
### Booleans
- `true`
- `false`

### Undefined
- `undefined`

This literal is used to explicitly denote that the assignee's value is undefined. Reading from undefined memory is illegal and safety checked when possible.

This can be assigned to any type, including as a call argument or member initializer. There are no restrictions on the usage of this literal for initialization.

## Numeric Literals
All numbers in ghoti can be broken into sections with an underscore. The underscore may be placed in between any two digits, meaning it can neither lead or end any part of a literal (exponent included). For example:
```ghoti
0b1100_0011;   // Legal
0b1100_0011_;  // Illegal
3.14_15_92;    // Legal
3_.14_15_92;   // Illegal
3.14_15_92_;   // Illegal
3e14;          // Legal
3_e14;         // Illegal
```

In all of these cases, the underscore has no impact on the resulting value of the numeric literal. Internally, the underscores are stripped out before constexpr evaluation and code generation.

It is recommended to use underscores in numbers when there is a logical grouping of digits for your use case. They are implemented in such a way that there is a high amount of freedom for the programmer to convey the most meaning with their code.

### Trailing Numeric Literals
To designate the true type of a numeric literal, you may use the following suffixes that were also discussed in the [type](syntax/types.md) documentation.

#### Integer Suffixes
| Suffix | Type    |
| ------ | ------- |
| None   | `i32`   |
| `l`    | `i64`   |
| `z`    | `isize` |
| `u`    | `u32`   |
| `ul`   | `u64`   |
| `uz`   | `usize` |

#### Float Suffixes
| Suffix | Type    |
| ------ | ------- |
| `f`    | `f32`   |
| None   | `f64`   |

### Integer Bases
Integer literals may be prefixed to denote a base:
| Prefix | Base    |
| ------ | ------- |
| None   | 10      |
| `0o`   | 8       |
| `0x`   | 16      |
| `0b`   | 2       |

Conformance to the bases digit set is enforced during tokenization.