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
    ├─ LHS: BinaryExpression (BOOLEAN_OR)
    │   ├─ LHS: BinaryExpression (LT_EQ)
    │   │   ├─ LHS: IdentifierExpression: a
    │   │   └─ RHS: IdentifierExpression: b
    │   └─ RHS: BinaryExpression (EQ)
    │       ├─ LHS: IdentifierExpression: c
    │       └─ RHS: IdentifierExpression: d
    └─ RHS: IdentifierExpression: e
ExpressionStatement
└─ Expr: BinaryExpression (BOOLEAN_OR)
    ├─ LHS: IdentifierExpression: a
    └─ RHS: BinaryExpression (EQ)
        ├─ LHS: IndexExpression
        │   ├─ Object: IdentifierExpression: b
        │   └─ Index: USizeIntegerExpression: 3
        └─ RHS: UnaryExpression (BANG)
            └─ Operand: IdentifierExpression: c
JumpStatement (CONTINUE)
JumpStatement (RETURN)
└─ Value: EnumExpression
    └─ Enumerations:
        └─ Name: IdentifierExpression: RED
ImportStatement
└─ Module: IdentifierExpression: std
ImportStatement
├─ User: StringExpression: ast/node.conch
└─ Alias: IdentifierExpression: node
DiscardStatement
└─ Discarded: EnumExpression
    └─ Enumerations:
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
    │   ├─ Assignee: IdentifierExpression: i
    │   └─ Value: SignedIntegerExpression: 1
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
    ├─ Matcher: IdentifierExpression: a
    ├─ Arms:
    │   ├─ Arm:
    │   │   ├─ Pattern: IdentifierExpression: b
    │   │   ├─ Capture: IdentifierExpression: c
    │   │   └─ Dispatch: ExpressionStatement
    │   │       └─ Expr: IdentifierExpression: d
    │   ├─ Arm:
    │   │   ├─ Pattern: IdentifierExpression: e
    │   │   ├─ Capture: <discarded>
    │   │   └─ Dispatch: ExpressionStatement
    │   │       └─ Expr: IdentifierExpression: f
    │   └─ Arm:
    │       ├─ Pattern: IdentifierExpression: g
    │       └─ Dispatch: ExpressionStatement
    │           └─ Expr: IdentifierExpression: h
    └─ Catch All: ExpressionStatement
        └─ Expr: IdentifierExpression: d
ExpressionStatement
└─ Expr: InfiniteLoopExpression
    └─ ExpressionStatement
        └─ Expr: IdentifierExpression: a
ExpressionStatement
└─ Expr: AssignmentExpression (ASSIGN)
    ├─ Assignee: DereferenceExpression (STAR)
    │   └─ Operand: IndexExpression
    │       ├─ Object: IndexExpression
    │       │   ├─ Object: IdentifierExpression: arr
    │       │   └─ Index: IdentifierExpression: i
    │       └─ Index: IdentifierExpression: j
    └─ Value: SignedIntegerExpression: 2
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
    │   └─ IdentifierExpression: int (primitive)
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
    ├─ Underlying: IdentifierExpression: ulong (primitive)
    └─ Enumerations:
        ├─ Name: IdentifierExpression: A
        │   └─ Default: UnsignedLongIntegerExpression: 1
        ├─ Name: IdentifierExpression: B
        │   └─ Default: IdentifierExpression: T
        └─ Name: IdentifierExpression: C
ExpressionStatement
└─ Expr: CallExpression
    ├─ Callee: IdentifierExpression: @ptrAdd (builtin)
    └─ Arguments: 
        ├─ IdentifierExpression: a
        └─ USizeIntegerExpression: 4
UsingStatement
├─ Alias: IdentifierExpression: T
└─ Type: ExplicitType (modifier: BASE)
    └─ IdentifierExpression: int (primitive)
ExpressionStatement
└─ Expr: CallExpression
    ├─ Callee: IdentifierExpression: a
    └─ Arguments: 
        ├─ ReferenceExpression (AND_MUT)
        │   └─ Operand: IdentifierExpression: r
        ├─ IdentifierExpression: t
        └─ ExplicitType (modifier: PTR)
            └─ ArrayType
                ├─ Dimensions: IdentifierExpression: N
                └─ ExplicitType (modifier: BASE)
                    └─ IdentifierExpression: int (primitive)
ExpressionStatement
└─ Expr: ImplicitDereferenceExpression (ARROW)
    ├─ Object: IdentifierExpression: a
    └─ Member: IdentifierExpression: b
ExpressionStatement
└─ Expr: ImplicitAccessExpression (DOT)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: UnionExpression
    ├─ Field:
    │   ├─ Tag: IdentifierExpression: a
    │   └─ Type: ExplicitType (modifier: BASE)
    │       └─ IdentifierExpression: int (primitive)
    └─ Field:
        ├─ Tag: IdentifierExpression: b
        └─ Type: ExplicitType (modifier: MUT_REF)
            └─ IdentifierExpression: T
)"