typedef struct Expr Expr;
typedef struct Stmnt Stmnt;
typedef struct Decl Decl;

typedef struct TypeSpec TypeSpec;
typedef struct FuncParam FuncParam;

// Enums   ================================================

typedef enum DeclType {
    DECL_NONE,
    DECL_ENUM,
    DECL_STRUCT,
    DECL_UNION,
    DECL_CONST,
    DECL_VAR,
    DECL_TYPEDEF,
    DECL_FUNC
} DeclType;

typedef enum ExprType {
    EXPR_NONE,
    EXPR_TERNARY,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_NAME,
    EXPR_COMPOUND,
    EXPR_CAST,
    EXPR_INDEX,
    EXPR_FIELD
} ExprType;

typedef enum StmntType {
    STMNT_NONE,
    STMNT_RETURN,
    STMNT_IF_ELSE,
    STMNT_SWITCH,
    STMNT_WHILE,
    STMNT_DO_WHILE,
    STMNT_FOR,
    STMNT_ASSIGN,
    STMNT_AUTO_ASSIGN,
    STMNT_BREAK,
    STMNT_CONTINUE,
    STMNT_BLOCK,
    STMNT_EXPR
} StmntType;

typedef enum TypeSpecType {
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_PTR
} TypeSpecType;

// ========================================================

// TypeSpecs ==============================================

typedef struct FuncTypeSpec {
    TypeSpec* ret_type;
    size_t num_params;
    TypeSpec** params;
} FuncTypeSpec;

typedef struct TypeSpec {
    TypeSpecType type;
    union {
        const char* name;
        // ptr, array
        struct {
            TypeSpec* base;
            Expr* size;
        };
        FuncTypeSpec func;
    };
} TypeSpec;

TypeSpec* typespec_alloc(TypeSpecType type) {
    TypeSpec* typespec = xcalloc(1, sizeof(TypeSpec));
    typespec->type = type;
    return typespec;
}

FuncTypeSpec* functypespec_alloc() {
    return xcalloc(1, sizeof(FuncTypeSpec));
}

TypeSpec* typespec_name(const char* name) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_NAME);
    typespec->name = name;
    return typespec;
}

TypeSpec* typespec_func(TypeSpec* ret_type, size_t num_params, TypeSpec** params) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_FUNC);
    FuncTypeSpec functypespec = {ret_type, num_params, params};
    typespec->func = functypespec;
    return typespec;
}

TypeSpec* typespec_array(TypeSpec* type, Expr* size) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_ARRAY);
    typespec->base = type;
    typespec->size = size;
    return typespec;
}

TypeSpec* typespec_ptr(TypeSpec* base) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_PTR);
    typespec->base = base;
    return typespec;
}

// ========================================================

// Decls ==================================================

typedef struct EnumItem {
    const char* name;
    TypeSpec* type;
} EnumItem;

typedef struct AggregateItem {
    size_t num_names;
    const char** names;
    TypeSpec* type;
} AggregateItem;

typedef struct FuncParam {
    const char* name;
    TypeSpec* type;
} FuncParam;

typedef struct FuncDecl {
    size_t num_params;
    FuncParam* params;
    TypeSpec* return_type;
} FuncDecl;

struct Decl {
    DeclType type;
    const char* name;
    union {
        struct {
            size_t num_enum_items;
            EnumItem* enum_items;
        };
        struct {
            size_t num_aggregate_items;
            AggregateItem* aggregate_items;
        };
        struct {
            TypeSpec* typespec;
            Expr* expr;
        };
        FuncDecl func_decl;
    };
};

// ========================================================

// Exprs ==================================================

struct Expr {
    ExprType type;
    TokenType op;
    union {
        uint64_t int_val;
        double float_val;
        const char* str_val;
        const char* name;
        // unary
        struct {
            Expr* uexpr;
            union {
                const char* ufield; // fields
                Expr* uindex; // index
                // argument list
                struct {
                    size_t num_uargs;
                    Expr** uargs;
                };
            };
        };
        // binary
        struct {
            Expr* bleft;
            Expr* bright;
        };
        // ternary
        struct {
            Expr* tcond;
            Expr* tleft;
            Expr* tright;
        };
        // compound
        struct {
            TypeSpec* compound_type;
            size_t num_compound_items;
            Expr** compound_items; // expr list
        };
        // cast
        struct {
            TypeSpec* cast_type;
            Expr* cast_expr;
        };
    };
};

Expr* expr_alloc(ExprType type) {
    Expr* expr = xcalloc(1, sizeof(Expr));
    expr->type = type;
    return expr;
}

Expr* expr_int(uint64_t int_val) {
    Expr* expr = expr_alloc(EXPR_INT);
    expr->int_val = int_val;
    return expr;
}

Expr* expr_float(double float_val) {
    Expr* expr = expr_alloc(EXPR_FLOAT);
    expr->float_val = float_val;
    return expr;
}

Expr* expr_str(const char* str_val) {
    Expr* expr = expr_alloc(EXPR_STR);
    expr->str_val = str_val;
    return expr;
}

Expr* expr_name(const char* name) {
    Expr* expr = expr_alloc(EXPR_NAME);
    expr->name = name;
    return expr;
}

Expr* expr_cast(TypeSpec* cast_type, Expr* cast_expr) {
    Expr* expr = expr_alloc(EXPR_CAST);
    expr->cast_type = cast_type;
    expr->cast_expr = cast_expr;
    return expr;
}

Expr* expr_unary(TokenType op, Expr* uexpr) {
    Expr* expr = expr_alloc(EXPR_UNARY);
    expr->op = op;
    expr->uexpr = uexpr;
    return expr;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right) {
    Expr* expr = expr_alloc(EXPR_BINARY);
    expr->op = op;
    expr->bleft = left;
    expr->bright = right;
    return expr;
}

Expr* expr_ternary(Expr* cond, Expr* left, Expr* right) {
    Expr* expr = expr_alloc(EXPR_TERNARY);
    expr->tcond = cond;
    expr->tleft = left;
    expr->tright = right;
    return expr;
}

Expr* expr_call(Expr* call_expr, size_t num_uargs, Expr** uargs) {
    Expr* expr = expr_alloc(EXPR_CALL);
    expr->uexpr = call_expr;
    expr->num_uargs = num_uargs;
    expr->uargs = uargs;
    return expr;
}

Expr* expr_index(Expr* call_expr, Expr* index) {
    Expr* expr = expr_alloc(EXPR_INDEX);
    expr->uexpr = call_expr;
    expr->uindex = index;
    return expr;
}

Expr* expr_field(Expr* call_expr, const char* field) {
    Expr* expr = expr_alloc(EXPR_FIELD);
    expr->uexpr = call_expr;
    expr->ufield = field;
    return expr;
}

Expr* expr_compound(TypeSpec* type, size_t num_compound_items, Expr** compound_items) {
    Expr* expr = expr_alloc(EXPR_COMPOUND);
    expr->compound_type = type;
    expr->num_compound_items = num_compound_items;
    expr->compound_items = compound_items;
    return expr;
}

// ========================================================

// Statements =============================================

typedef struct ElseIfItem {
    Expr* cond;
    Stmnt* stmnt;
} ElseIfItem;

typedef struct ElseItem {
    Stmnt* stmnt;
} ElseItem;

typedef struct CaseBlock {
    union {
        uint64_t int_val;
        double float_val;
        const char* str_val;
    };
    Stmnt* stmnt;
} CaseBlock;

typedef struct DefaultBlock {
    Stmnt* stmnt;
} DefaultBlock;

struct Stmnt {
    StmntType type;
    union {
        // if else
        struct {
            Expr* if_cond;
            size_t num_else_ifs;
            ElseIfItem* else_ifs;
            ElseItem else_item;
        };
        // switch
        struct {
            Expr* switch_cond;
            size_t num_case_blocks;
            CaseBlock* case_blocks;
            DefaultBlock default_block;
        };
        // while
        struct {
            Expr* while_cond;
            Stmnt* while_stmnt;
        };
        // for
        struct {
            Expr** for_init;
            Expr* for_cond;
            Expr** for_update;
            Stmnt* for_stmnt;
        };
        // assign, auto assign
        struct {
            Expr* lval;
            Expr* rval;
        };
        // stmnt block
        struct {
            size_t num_stmnts;
            Stmnt** stmnts;
        };
        // expr, return
        struct {
            Expr* expr;
        };
    };
};

// ========================================================

// tests ==================================================

void print_expr(Expr*);

void print_typespec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME:
        printf("(%s)", typespec->name);
        break;
    case TYPESPEC_FUNC:
        printf("((");
        print_typespec(typespec->func.ret_type);
        printf(")");
        printf("(");
        for (size_t i = 0; i < typespec->func.num_params; i++) {
            print_typespec(typespec->func.params[i]);
            printf(",");
        }
        printf("))");
        break;
    case TYPESPEC_ARRAY:
        printf("((");
        print_typespec(typespec->base);
        printf(")[");
        print_expr(typespec->size);
        printf("])");
        break;
    case TYPESPEC_PTR:
        printf("((");
        print_typespec(typespec->base);
        printf(")*)");
        break;
        break;
    default:
        assert(0);
    }
}

void print_expr(Expr* expr) {
    switch (expr->type) {
    case EXPR_TERNARY:
        printf("(");
        print_expr(expr->tcond);
        printf(")?(");
        print_expr(expr->tleft);
        printf("):(");
        print_expr(expr->tright);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(");
        print_expr(expr->bleft);
        printf(")%c(", expr->op);
        print_expr(expr->bright);
        printf(")");
        break;
    case EXPR_UNARY:
        printf("%c(", expr->op);
        print_expr(expr->uexpr);
        printf(")");
        break;
    case EXPR_CALL:
        printf("(");
        print_expr(expr->uexpr);
        printf(")(");
        for (size_t i = 0; i < expr->num_uargs; i++) {
            print_expr(expr->uargs[i]);
            printf(",");
        }
        printf(")");
        break;
    case EXPR_INT:
        printf("%llu", expr->int_val);
        break;
    case EXPR_FLOAT:
        printf("%f", expr->float_val);
        break;
    case EXPR_STR:
        printf("\"%s\"", expr->str_val);
        break;
    case EXPR_NAME:
        printf("%s", expr->name);
        break;
    case EXPR_COMPOUND:
        printf("{");
        for (size_t i = 0; i < expr->num_compound_items; i++) {
            print_expr(expr->compound_items[i]);
            printf(",");
        }
        printf("}");
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_typespec(expr->cast_type);
        printf(" ");
        print_expr(expr->cast_expr);
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(");
        print_expr(expr->uexpr);
        printf(")[");
        print_expr(expr->uindex);
        printf("]");
        break;
    case EXPR_FIELD:
        printf("(");
        print_expr(expr->uexpr);
        printf(").%s", expr->ufield);
        break;
    default:
        assert(0);
    }
}

print_expr_line(Expr* expr) {
    print_expr(expr);
    printf("\n");
}

print_typespec_line(TypeSpec* typespec) {
    print_typespec(typespec);
    printf("\n");
}

#define CREATE_PRINT_EXPR(e, create_expr) Expr* (e) = (create_expr); print_expr_line(e)
#define CREATE_PRINT_TYPE(t, create_type) TypeSpec* (t) = (create_type); print_typespec_line(t)
ast_test() {
    CREATE_PRINT_TYPE(t1, typespec_name("list"));
    CREATE_PRINT_TYPE(t2, typespec_array(t1, expr_binary('+', expr_int(1), expr_int(2))));
    CREATE_PRINT_TYPE(t3, typespec_ptr(t2));
    CREATE_PRINT_TYPE(t4, typespec_func(t3, 2, (TypeSpec*[]) { typespec_name("int"), t2 }));

    CREATE_PRINT_EXPR(e1, expr_binary('+', expr_int(41), expr_int(2)));
    CREATE_PRINT_EXPR(e2, expr_binary('-', expr_int(1), expr_int(221)));
    CREATE_PRINT_EXPR(e3, expr_binary('^', e1, e2));
    CREATE_PRINT_EXPR(e4, expr_unary('!', e3));
    CREATE_PRINT_EXPR(e5, expr_ternary(e4, expr_call(expr_name("foo"), 2, (Expr*[]){expr_int(2), expr_str("bar")}), expr_int(2)));
    CREATE_PRINT_EXPR(e6, expr_field(expr_cast(typespec_name("vector"),
        expr_compound(typespec_name("int"), 2, (Expr*[]) {expr_str("a"), expr_str("b")})), "size"));
    CREATE_PRINT_EXPR(e7, expr_index(expr_compound(t2, 1, (Expr*[]) {expr_int(2)}), expr_call(expr_name("foo"), 0, NULL)));

    printf("ast test passed");
}
#undef CREATE_PRINT_EXPR
#undef CREATE_PRINT_TYPE

// ========================================================
