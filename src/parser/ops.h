#if !defined(_ALL_OPS) && !defined(_BINARY_OPS) && !defined(_UNARY_OPS) 
    #define _ALL_OPS
#endif

#if defined(_ALL_OPS)
    #define _BINARY_OPS
    #define _UNARY_OPS
#endif

//# _DEF(OP)
#ifdef _DEF

#ifdef _BINARY_OPS
_DEF(BINARY_OP_ADD),
_DEF(BINARY_OP_SUBTRACT),
_DEF(BINARY_OP_MULTIPLY),
_DEF(BINARY_OP_DIVIDE),
_DEF(BINARY_OP_MODULO),

_DEF(BINARY_OP_EQUAL),
_DEF(BINARY_OP_NOT_EQUAL),
_DEF(BINARY_OP_AND),
_DEF(BINARY_OP_OR),

/* left: the array, right: the index */
_DEF(BINARY_OP_ARRAY_INDEX),
/* left: <idf: struct>, right: <idf: member> */
_DEF(BINARY_OP_GET_MEMBER),

_DEF(BINARY_OP_ASSIGN),
#endif

#ifdef _UNARY_OPS
_DEF(UNARY_OP_ADDRESS_OF),
_DEF(UNARY_OP_DEREFERENCE),
#endif

#if defined(_ALL_OPS)
_DEF(OP_PAREN_OPEN),
_DEF(OP_PAREN_CLOSE),
_DEF(OP_INVALID),
_DEF(OP_COUNT)
#endif  

#endif

#if defined(_ALL_OPS)
    #undef _ALL_OPS
#endif

#if defined(_BINARY_OPS)
    #undef _BINARY_OPS
#endif

#if defined(_UNARY_OPS)
    #undef _UNARY_OPS
#endif
