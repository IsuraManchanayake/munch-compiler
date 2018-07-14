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
    TYPESPEC_NAME,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_PTR
} TypeSpecType;

// ========================================================

// TypeSpecs ==============================================

typedef struct FuncTypeSpec {
    TypeSpec* ret_type;
    TypeSpec** params; // sbuf
} FuncTypeSpec;

typedef struct TypeSpec {
    TypeSpecType type;
    union {
        const char* name;
        // ptr, array
        struct {
            TypeSpec* typespec;
            Expr* index;
        };
        FuncTypeSpec func;
    };
} TypeSpec;

// ========================================================

// Decls ==================================================

typedef struct EnumItem {
    const char* name;
    TypeSpec* type;
} EnumItem;

typedef struct AggregateItem {
    const char** names; // sbuf
    TypeSpec* type;
} AggregateItem;

typedef struct FuncParam {
    const char* name;
    TypeSpec* type;
} FuncParam;

typedef struct FuncDecl {
    FuncParam* params; // sbuf
    TypeSpec* return_type;
} FuncDecl;

struct Decl {
    DeclType type;
    const char* name;
    union {
        EnumItem* enum_items; // sbuf
        AggregateItem* aggregate_items; // sbuf
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
                Expr** uargs; // argument list // sbuf
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
            Expr** compound_list; // expr list // sbuf
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
            ElseIfItem* else_ifs; // sbuf;
            ElseItem else_item;
        };
        // switch
        struct {
            Expr* switch_cond;
            CaseBlock* case_blocks; // sbuf 
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
            Stmnt** stmnts; // sbuf
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

void print_type_spec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME:
        printf("(%s)", typespec->name);
        break;
    case TYPESPEC_FUNC:
        printf("((");
        print_type_spec(typespec->func.ret_type);
        printf(")");
        printf("(");
        for (int i = 0; i < buf_len(typespec->func.params); i++) {
            print_type_spec(typespec->func.params[i]);
            printf(",");
        }
        printf("))");
        break;
    case TYPESPEC_ARRAY:
        printf("((");
        print_type_spec(typespec->typespec);
        printf(")[");
        print_expr(typespec->index);
        printf("])");
        break;
    case TYPESPEC_PTR:
        printf("((");
        print_type_spec(typespec->typespec);
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
        printf("(");
        print_expr(expr->uexpr);
        printf(")%c", expr->op);
        break;
    case EXPR_CALL:
        printf("(");
        print_expr(expr->uexpr);
        printf(")(");
        for (size_t i = 0; i < buf_len(expr->uargs); i++) {
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
        for (size_t i = 0; i < buf_len(expr->compound_list); i++) {
            print_expr(expr->compound_list[i]);
            printf(",");
        }
        printf("}");
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_type_spec(expr->cast_type);
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
    default:
        assert(0);
    }
}

ast_test() {
    Expr* expr1 = expr_int(32);
    Expr* expr2 = expr_int(51);
    Expr* expr3 = expr_binary('+', expr1, expr2);
    Expr* expr4 = expr_binary('+', expr3, expr2);
    print_expr(expr1);
    printf("\n");
    print_expr(expr2);
    printf("\n");
    print_expr(expr3);
    printf("\n");
    print_expr(expr4);
    printf("\n");
    free(expr1);
    free(expr2);
    free(expr3);
    free(expr4);
}

// ========================================================
