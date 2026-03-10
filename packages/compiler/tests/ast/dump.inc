R"(ExpressionStatement
└─ Expr: ArrayExpression
    ├─ Size: (inferred)
    ├─ Type: ExplicitType (modifier: PTR)
    │   └─ IdentifierExpression: N
    └─ Items: 
        ├─ IdentifierExpression: a
        ├─ IdentifierExpression: b
        ├─ IdentifierExpression: c
        ├─ IdentifierExpression: d
        ├─ IdentifierExpression: e
        ├─ SignedIntegerExpression: 3
        └─ StringExpression: 54
ExpressionStatement
└─ Expr: BinaryExpression (BOOLEAN_AND)
    ├─ LHS BinaryExpression (BOOLEAN_OR)
    │   ├─ LHS BinaryExpression (LT_EQ)
    │   │   ├─ LHS IdentifierExpression: a
    │   │   └─ RHS IdentifierExpression: b
    │   └─ RHS BinaryExpression (EQ)
    │       ├─ LHS IdentifierExpression: c
    │       └─ RHS IdentifierExpression: d
    └─ RHS IdentifierExpression: e
ExpressionStatement
└─ Expr: BinaryExpression (BOOLEAN_OR)
    ├─ LHS IdentifierExpression: a
    └─ RHS BinaryExpression (EQ)
        ├─ LHS IndexExpression
        │   ├─ Object: IdentifierExpression: b
        │   └─ Index: USizeIntegerExpression: 3
        └─ RHS UnaryExpression (BANG)
            └─ Operand: IdentifierExpression: c
JumpStatement (CONTINUE)
JumpStatement (RETURN)
└─ Value: EnumExpression
    └─ Variants:
        └─ Name: IdentifierExpression: RED
ImportStatement
└─ Module: IdentifierExpression: std
ImportStatement
├─ User: StringExpression: ast/node.conch
└─ Alias: IdentifierExpression: node
DiscardStatement
└─ Discarded: EnumExpression
    └─ Variants:
        └─ Name: IdentifierExpression: RED
DeclStatement (SIZE)
├─ Modifiers: COMPTIME
├─ Type: (inferred)
└─ Value: USizeIntegerExpression: 2
BlockStatement
├─ ExpressionStatement
│   └─ Expr: IdentifierExpression: a
├─ ExpressionStatement
│   └─ Expr: IdentifierExpression: b
├─ ExpressionStatement
│   └─ Expr: SignedIntegerExpression: 2
└─ ExpressionStatement
    └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: WhileLoopExpression
    ├─ Condition: BoolExpression: true
    ├─ Continuation: AssignmentExpression (PLUS_ASSIGN)
    │   ├─ Assignee IdentifierExpression: i
    │   └─ Value SignedIntegerExpression: 1
    ├─ Body: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: a
    └─ Non-Break Clause: JumpStatement (RETURN)
        └─ Value: IdentifierExpression: b
DeclStatement (f_ptr)
├─ Modifiers: VARIABLE
└─ Type: ExplicitType (modifier: PTR)
    └─ FunctionExpression
        ├─ Self: a (modifier: REF)
        ├─ Parameters:
        │   └─ Param: 
        │       ├─ Name: IdentifierExpression: b
        │       └─ Type: ExplicitType (modifier: MUT_PTR)
        │           └─ IdentifierExpression: B
        └─ Returns: ExplicitType (modifier: REF)
            └─ ArrayType
                ├─ Dimensions: USizeIntegerExpression: 2
                └─ ExplicitType (modifier: BASE)
                    └─ ArrayType
                        ├─ Dimensions: IdentifierExpression: N
                        └─ ExplicitType (modifier: PTR)
                            └─ IdentifierExpression: E
ExpressionStatement
└─ Expr: ScopeResolutionExpression
    ├─ Outer: ScopeResolutionExpression
    │   ├─ Outer: IdentifierExpression: A
    │   └─ Inner: IdentifierExpression: B
    └─ Inner: IdentifierExpression: C
ExpressionStatement
└─ Expr: StructExpression (packed)
    ├─ DeclStatement (a)
    │   ├─ Modifiers: VARIABLE
    │   ├─ Type: ExplicitType (modifier: BASE)
    │   │   └─ IdentifierExpression: Foo
    │   └─ Value: IdentifierExpression: bar
    └─ DeclStatement (b)
        ├─ Modifiers: CONSTANT
        ├─ Type: (inferred)
        └─ Value: FunctionExpression
            ├─ Self: this (modifier: MUT_PTR)
            ├─ Parameters:
            │   ├─ Param: 
            │   │   ├─ Name: IdentifierExpression: a
            │   │   └─ Type: ExplicitType (modifier: BASE)
            │   │       └─ IdentifierExpression: A
            │   └─ Param: 
            │       ├─ Name: IdentifierExpression: b
            │       └─ Type: ExplicitType (modifier: PTR)
            │           └─ IdentifierExpression: B
            ├─ Returns: ExplicitType (modifier: BASE)
            │   └─ IdentifierExpression: C
            └─ Body: BlockStatement
                └─ ExpressionStatement
                    └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: ReferenceExpression (BW_AND)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: ReferenceExpression (AND_MUT)
    └─ Operand: IdentifierExpression: b
ExpressionStatement
└─ Expr: DereferenceExpression (STAR)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: MatchExpression
    ├─ Input: IdentifierExpression: a
    ├─ Arms:
    │   └─ Arm
    │       ├─ Pattern: IdentifierExpression: b
    │       └─ Dispatch: BlockStatement
    │           └─ ExpressionStatement
    │               └─ Expr: IdentifierExpression: c
    └─ Catch All: ExpressionStatement
        └─ Expr: IdentifierExpression: d
ExpressionStatement
└─ Expr: InfiniteLoopExpression
    └─ ExpressionStatement
        └─ Expr: IdentifierExpression: a
ExpressionStatement
└─ Expr: AssignmentExpression (ASSIGN)
    ├─ Assignee DereferenceExpression (STAR)
    │   └─ Operand: IndexExpression
    │       ├─ Object: IndexExpression
    │       │   ├─ Object: IdentifierExpression: arr
    │       │   └─ Index: IdentifierExpression: i
    │       └─ Index: IdentifierExpression: j
    └─ Value SignedIntegerExpression: 2
ExpressionStatement
└─ Expr: IfExpression
    ├─ Condition: IdentifierExpression: a
    ├─ Then:BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: b
    └─ Else:BlockStatement
        └─ ExpressionStatement
            └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: FunctionExpression
    ├─ Self: this (modifier: MUT_PTR)
    ├─ Parameters:
    │   ├─ Param: 
    │   │   ├─ Name: IdentifierExpression: a
    │   │   └─ Type: ExplicitType (modifier: BASE)
    │   │       └─ IdentifierExpression: A
    │   └─ Param: 
    │       ├─ Name: IdentifierExpression: b
    │       └─ Type: ExplicitType (modifier: PTR)
    │           └─ IdentifierExpression: B
    ├─ Returns: ExplicitType (modifier: BASE)
    │   └─ IdentifierExpression: int
    └─ Body: BlockStatement
        └─ ExpressionStatement
            └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: ForLoopExpression
    ├─ Iterables:
    │   ├─ IdentifierExpression: arr
    │   ├─ IdentifierExpression: l
    │   └─ IdentifierExpression: p
    ├─ Captures:
    │   ├─ i (modifier: BASE)
    │   ├─ j (modifier: MUT_REF)
    │   └─ <discarded>
    ├─ Body: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: a
    └─ Non-Break: JumpStatement (RETURN)
        └─ Value: IdentifierExpression: b
ExpressionStatement
└─ Expr: EnumExpression
    ├─ Underlying: IdentifierExpression: ulong
    └─ Variants:
        ├─ Name: IdentifierExpression: A
        │   └─ Default: UnsignedLongIntegerExpression: 1
        ├─ Name: IdentifierExpression: B
        │   └─ Default: IdentifierExpression: T
        └─ Name: IdentifierExpression: C
ExpressionStatement
└─ Expr: CallExpression
    ├─ Callee: IdentifierExpression: @ptrAdd
    └─ Arguments: 
        ├─ IdentifierExpression: a
        └─ USizeIntegerExpression: 4
)"