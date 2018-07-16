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

typedef struct NameTypeSpec {
	const char* name;
} NameTypeSpec;

typedef struct FuncTypeSpec {
    TypeSpec* ret_type;
    size_t num_params;
    TypeSpec** params;
} FuncTypeSpec;

typedef struct ArrayTypeSpec {
	TypeSpec* base;
	Expr* size;
} ArrayTypeSpec;

typedef struct PtrTypeSpec {
	TypeSpec* base;
} PtrTypeSpec;

// --------------------------------------------------------

typedef struct TypeSpec {
    TypeSpecType type;
    union {
		NameTypeSpec name;
		FuncTypeSpec func;
		ArrayTypeSpec array;
		PtrTypeSpec ptr;
    };
} TypeSpec;

// --------------------------------------------------------

TypeSpec* typespec_alloc(TypeSpecType type) {
    TypeSpec* typespec = xcalloc(1, sizeof(TypeSpec));
    typespec->type = type;
    return typespec;
}

TypeSpec* typespec_name(const char* name) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_NAME);
	typespec->name = (NameTypeSpec) { .name=name };
    return typespec;
}

TypeSpec* typespec_func(TypeSpec* ret_type, size_t num_params, TypeSpec** params) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_FUNC);
    typespec->func = (FuncTypeSpec) { .ret_type=ret_type, .num_params=num_params, .params=params };
    return typespec;
}

TypeSpec* typespec_array(TypeSpec* base, Expr* size) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_ARRAY);
	typespec->array = (ArrayTypeSpec) {.base=base, .size=size};
    return typespec;
}

TypeSpec* typespec_ptr(TypeSpec* base) {
    TypeSpec* typespec = typespec_alloc(TYPESPEC_PTR);
	typespec->ptr = (PtrTypeSpec) {.base=base};
    return typespec;
}

// ========================================================

// Decls ==================================================

typedef struct EnumItem {
    const char* name;
	uint64_t int_val;
} EnumItem;

typedef struct EnumDecl {
	size_t num_enum_items;
	EnumItem* enum_items;
} EnumDecl;

typedef struct ConstDecl {
	Expr* expr;
} ConstDecl;

typedef struct VarDecl {
	TypeSpec* type;
	Expr* expr;
} VarDecl;

typedef struct TypedefDecl {
	TypeSpec* type;
} TypedefDecl;

typedef struct AggregateItem {
	const char* name;
	TypeSpec* type;
	Expr* expr;
} AggregateItem;

typedef struct AggregateDecl {
	size_t num_aggregate_items;
	AggregateItem* aggregate_items;
} AggregateDecl;

typedef struct FuncParam {
	const char* name;
	TypeSpec* type;
} FuncParam;

typedef struct FuncDecl {
    size_t num_params;
    FuncParam* params;
    TypeSpec* ret_type;
} FuncDecl;

// --------------------------------------------------------

struct Decl {
    DeclType type;
    const char* name;
    union {
		EnumDecl enum_decl;
		AggregateDecl aggregate_decl;
		ConstDecl const_decl;
		VarDecl var_decl;
		TypedefDecl typedef_decl;
        FuncDecl func_decl;
    };
};

// --------------------------------------------------------

Decl* decl_alloc(DeclType type, const char* name) {
	Decl* decl = xcalloc(1, sizeof(Decl));
	decl->type = type;
	decl->name = name;
	return decl;
}

Decl* decl_enum(const char* name, size_t num_enum_items, EnumItem* enum_items) {
	Decl* decl = decl_alloc(DECL_ENUM, name);
	decl->enum_decl = (EnumDecl) { .num_enum_items=num_enum_items, .enum_items=enum_items};
	return decl;
}

Decl* decl_aggregate(DeclType type, const char* name, size_t num_aggregate_items, AggregateItem* aggregate_items) {
	assert(type == DECL_UNION || type == DECL_STRUCT);
	Decl* decl = decl_alloc(type, name);
	decl->aggregate_decl = (AggregateDecl) { .num_aggregate_items = num_aggregate_items, .aggregate_items = aggregate_items };
	return decl;
}

Decl* decl_const(const char* name, Expr* expr) {
	Decl* decl = decl_alloc(DECL_CONST, name);
	decl->const_decl = (ConstDecl) { .expr = expr };
	return decl;
}

Decl* decl_var(const char* name, TypeSpec* type, Expr* expr) {
	Decl* decl = decl_alloc(DECL_VAR, name);
	decl->var_decl = (VarDecl) { .type = type, .expr = expr };
	return decl;
}

Decl* decl_typedef(const char* name, TypeSpec* type) {
	Decl* decl = decl_alloc(DECL_TYPEDEF, name);
	decl->typedef_decl = (TypedefDecl) { .type = type };
	return decl;
}

Decl* decl_func(const char* name, size_t num_params, FuncParam* params, TypeSpec* ret_type) {
	Decl* decl = decl_alloc(DECL_FUNC, name);
	decl->func_decl = (FuncDecl) { .num_params = num_params, .params = params, .ret_type = ret_type };
	return decl;
}

// ========================================================

// Exprs ==================================================

typedef struct TernaryExpr {
	Expr* cond;
	Expr* left;
	Expr* right;
} TernaryExpr ;

typedef struct BinaryExpr {
	Expr* left;
	Expr* right;
	TokenType op;
} BinaryExpr;

typedef struct UnaryExpr {
	Expr* expr;
	TokenType op;
} UnaryExpr;

typedef struct CallExpr {
	Expr* expr;
	size_t num_args;
	Expr** args;
} CallExpr;

typedef struct IntExpr {
	uint64_t int_val;
} IntExpr;

typedef struct FloatExpr {
	double float_val;
} FloatExpr;

typedef struct StrExpr {
	const char* str_val;
} StrExpr;

typedef struct NameExpr {
	const char* name;
} NameExpr;

typedef struct CompoundExpr {
	TypeSpec* type;
	size_t num_exprs;
	Expr** exprs;
} CompoundExpr;

typedef struct CastExpr {
	Expr* cast_expr;
	TypeSpec* cast_type;
} CastExpr;

typedef struct IndexExpr {
	Expr* expr;
	Expr* index;
} IndexExpr;

typedef struct FieldExpr {
	Expr* expr;
	const char* field;
} FieldExpr;

// --------------------------------------------------------

struct Expr {
    ExprType type;
    union {
		TernaryExpr ternary_expr;
		BinaryExpr binary_expr;
		UnaryExpr unary_expr;
		CallExpr call_expr;
		IntExpr int_expr;
		FloatExpr float_expr;
		StrExpr str_expr;
		NameExpr name_expr;
		CompoundExpr compound_expr;
		CastExpr cast_expr;
		IndexExpr index_expr;
		FieldExpr field_expr;
    };
};

// --------------------------------------------------------

Expr* expr_alloc(ExprType type) {
    Expr* expr = xcalloc(1, sizeof(Expr));
    expr->type = type;
    return expr;
}

Expr* expr_int(uint64_t int_val) {
    Expr* expr = expr_alloc(EXPR_INT);
	expr->int_expr = (IntExpr) {.int_val=int_val};
    return expr;
}

Expr* expr_float(double float_val) {
    Expr* expr = expr_alloc(EXPR_FLOAT);
	expr->float_expr = (FloatExpr) {.float_val=float_val};
    return expr;
}

Expr* expr_str(const char* str_val) {
    Expr* expr = expr_alloc(EXPR_STR);
	expr->str_expr = (StrExpr) {.str_val=str_val};
    return expr;
}

Expr* expr_name(const char* name) {
    Expr* expr = expr_alloc(EXPR_NAME);
	expr->name_expr = (NameExpr) {.name=name};
    return expr;
}

Expr* expr_cast(TypeSpec* cast_type, Expr* cast_expr) {
    Expr* expr = expr_alloc(EXPR_CAST);
	expr->cast_expr = (CastExpr) { .cast_expr=cast_expr, .cast_type=cast_type };
    return expr;
}

Expr* expr_unary(TokenType op, Expr* uexpr) {
	assert(op == TOKEN_INC || op == TOKEN_DEC || op == '+'
		|| op == '-' || op == '*' || op == '!' || op == '~');
    Expr* expr = expr_alloc(EXPR_UNARY);
	expr->unary_expr = (UnaryExpr) { .expr=uexpr, .op=op };
    return expr;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right) {
	assert(op == TOKEN_LOG_AND || op == TOKEN_LOG_OR || op == TOKEN_LSHIFT 
		|| op == TOKEN_RSHIFT || op == TOKEN_EQ || op == TOKEN_NEQ
		|| op == TOKEN_LTEQ || op == TOKEN_GTEQ || op == '+'
		|| op == '-' || op == '*' || op == '/'
		|| op == '%' || op == '<' || op == '>'
		|| op == '&' || op == '|' || op == '^');
    Expr* expr = expr_alloc(EXPR_BINARY);
	expr->binary_expr = (BinaryExpr) { .left=left, .right=right, .op=op };
    return expr;
}

Expr* expr_ternary(Expr* cond, Expr* left, Expr* right) {
    Expr* expr = expr_alloc(EXPR_TERNARY);
	expr->ternary_expr = (TernaryExpr) { .cond=cond, .left=left, .right=right };
    return expr;
}

Expr* expr_call(Expr* call_expr, size_t num_args, Expr** args) {
    Expr* expr = expr_alloc(EXPR_CALL);
	expr->call_expr = (CallExpr) { .expr=call_expr, .num_args=num_args, .args=args };
    return expr;
}

Expr* expr_index(Expr* call_expr, Expr* index) {
    Expr* expr = expr_alloc(EXPR_INDEX);
	expr->index_expr = (IndexExpr) { .expr=call_expr, .index=index };
    return expr;
}

Expr* expr_field(Expr* call_expr, const char* field) {
    Expr* expr = expr_alloc(EXPR_FIELD);
	expr->field_expr = (FieldExpr) { .expr=call_expr, .field=field };
    return expr;
}

Expr* expr_compound(TypeSpec* type, size_t num_exprs, Expr** exprs) {
    Expr* expr = expr_alloc(EXPR_COMPOUND);
	expr->compound_expr = (CompoundExpr) { .type=type, .num_exprs=num_exprs, .exprs=exprs };
    return expr;
}

// ========================================================

// Statements =============================================

typedef struct ElseIfItem {
	Expr* cond;
	Stmnt* stmnt;
} ElseIfItem;

typedef struct IfElseIfStmnt {
	Expr* if_cond;
	Stmnt* then_stmnt;
	size_t num_else_ifs;
	ElseIfItem* else_ifs;
	Stmnt* else_stmnt;
} IfElseIfStmnt;

typedef struct CaseBlock {
	Expr* case_expr;
	Stmnt* stmnt;
} CaseBlock;

typedef struct SwitchStmnt {
	Expr* switch_expr;
	size_t num_case_blocks;
	CaseBlock* case_blocks;
	Stmnt* default_stmnt;
} SwitchStmnt;

typedef struct WhileStmnt {
	Expr* cond;
	Stmnt* stmnt;
} WhileStmnt;

typedef struct ForStmnt {
	Expr** init;
	Expr* cond;
	Expr** update;
	Stmnt* stmnt;
} ForStmnt;

typedef struct AssignStmnt {
	Expr* left;
	Expr* right;
	TokenType op;
} AssignStmnt;

typedef struct BlockStmnt {
	size_t num_stmnts;
	Stmnt** stmnts;
} BlockStmnt;

typedef struct ReturnStmnt {
	Expr* expr;
} ReturnStmnt;

// --------------------------------------------------------

struct Stmnt {
    StmntType type;
    union {
		IfElseIfStmnt ifelseif_stmnt;
		SwitchStmnt switch_stmnt;
		WhileStmnt while_stmnt;
		ForStmnt for_stmnt;
		AssignStmnt assign_stmnt;
		BlockStmnt block_stmnt;
		ReturnStmnt return_stmnt;
    };
};

// --------------------------------------------------------

Stmnt* stmnt_alloc(StmntType type) {
	Stmnt* stmnt = xcalloc(1, sizeof(Stmnt));
	stmnt->type = type;
	return stmnt;
}

Stmnt* return_stmnt(Expr* expr) {
	Stmnt* stmnt = stmnt_alloc(STMNT_RETURN);
	stmnt->return_stmnt = (ReturnStmnt) { expr };
	return stmnt;
}

Stmnt* ifelseif_stmnt(Expr* if_cond, Stmnt* then_stmnt, size_t num_else_ifs, ElseIfItem* else_ifs, Stmnt* else_stmnt) {
	Stmnt* stmnt = stmnt_alloc(STMNT_IF_ELSE);
	stmnt->ifelseif_stmnt = (IfElseIfStmnt) {
		.if_cond=if_cond, 
		.then_stmnt=then_stmnt, 
		.num_else_ifs=num_else_ifs,
		.else_ifs=else_ifs, 
		.else_stmnt=else_stmnt
	};
	return stmnt;
}

Stmnt* switch_stmnt(Expr* switch_expr, size_t num_case_blocks, CaseBlock* case_blocks, Stmnt* default_stmnt) {
	Stmnt* stmnt = stmnt_alloc(STMNT_SWITCH);
	stmnt->switch_stmnt = (SwitchStmnt) { 
		.switch_expr = switch_expr, 
		.num_case_blocks = num_case_blocks,
		.case_blocks = case_blocks,
		.default_stmnt = default_stmnt 
	};
	return stmnt;
}

Stmnt* while_stmnt(StmntType type, Expr* cond, Stmnt* body) {
	assert(type == STMNT_WHILE || type == STMNT_DO_WHILE);
	Stmnt* stmnt = stmnt_alloc(type);
	stmnt->while_stmnt = (WhileStmnt) { .cond = cond, .stmnt = body };
	return stmnt;
}

Stmnt* for_stmnt(Expr** init, Expr* cond, Expr** update, Stmnt* body) {
	Stmnt* stmnt = stmnt_alloc(STMNT_FOR);
	stmnt->for_stmnt = (ForStmnt) { .init = init, .cond = cond, .update = update, .stmnt = body };
	return stmnt;
}

Stmnt* assign_stmnt(Expr* left, Expr* right, TokenType op) {
	assert(op == TOKEN_ADD_ASSIGN || op == TOKEN_BIT_AND_ASSIGN || op == TOKEN_BIT_OR_ASSIGN 
		|| op == TOKEN_BIT_XOR_ASSIGN || op == TOKEN_COLON_ASSIGN || op == TOKEN_DIV_ASSIGN  
		|| op == TOKEN_MUL_ASSIGN || op == TOKEN_MOD_ASSIGN || op == TOKEN_LSHIFT_ASSIGN 
		|| op == TOKEN_RSHIFT_ASSIGN || op == TOKEN_ADD_ASSIGN || op == TOKEN_SUB_ASSIGN);
	Stmnt* stmnt = stmnt_alloc(STMNT_ASSIGN);
	stmnt->assign_stmnt = (AssignStmnt) { .left = left, .right = right, .op = op };
	return stmnt;
}

Stmnt* break_stmnt() {
	return stmnt_alloc(STMNT_BREAK);
}

Stmnt* continue_stmnt() {
	return stmnt_alloc(STMNT_CONTINUE);
}

Stmnt* block_stmnt(size_t num_stmnts, Stmnt** stmnts) {
	Stmnt* stmnt = stmnt_alloc(STMNT_BLOCK);
	stmnt->block_stmnt.num_stmnts = num_stmnts;
	stmnt->block_stmnt.stmnts = stmnts;
	return stmnt;
}

// ========================================================

// tests ==================================================

void print_expr(Expr*);

void print_typespec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME:
        printf("%s", typespec->name.name);
        break;
    case TYPESPEC_FUNC:
        printf("((");
        print_typespec(typespec->func.ret_type);
        printf(")(");
        for (size_t i = 0; i < typespec->func.num_params; i++) {
            print_typespec(typespec->func.params[i]);
            printf(",");
        }
        printf("))");
        break;
    case TYPESPEC_ARRAY:
        printf("(");
        print_typespec(typespec->array.base);
        printf("[");
        print_expr(typespec->array.size);
        printf("])");
        break;
    case TYPESPEC_PTR:
        printf("(ref ");
        print_typespec(typespec->ptr.base);
        printf(")");
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
        print_expr(expr->ternary_expr.cond);
        printf(")?(");
        print_expr(expr->ternary_expr.left);
        printf("):(");
        print_expr(expr->ternary_expr.right);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(");
        printf("%c ", expr->binary_expr.op);
		print_expr(expr->binary_expr.left);
		printf(" ");
        print_expr(expr->binary_expr.right);
        printf(")");
        break;
    case EXPR_UNARY:
        printf("(%c ", expr->unary_expr.op);
        print_expr(expr->unary_expr.expr);
        printf(")");
        break;
    case EXPR_CALL:
        printf("(call ");
        print_expr(expr->call_expr.expr);
        printf(" (");
        for (size_t i = 0; i < expr->call_expr.num_args; i++) {
            print_expr(expr->call_expr.args[i]);
            printf(",");
        }
        printf("))");
        break;
    case EXPR_INT:
        printf("%llu", expr->int_expr.int_val);
        break;
    case EXPR_FLOAT:
        printf("%f", expr->float_expr.float_val);
        break;
    case EXPR_STR:
        printf("\"%s\"", expr->str_expr.str_val);
        break;
    case EXPR_NAME:
        printf("%s", expr->name_expr.name);
        break;
    case EXPR_COMPOUND:
		printf("((");
		print_typespec(expr->compound_expr.type);
		printf(")");
        printf("{");
        for (size_t i = 0; i < expr->compound_expr.num_exprs; i++) {
            print_expr(expr->compound_expr.exprs[i]);
            printf(",");
        }
        printf("})");
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_typespec(expr->cast_expr.cast_type);
        printf(" ");
        print_expr(expr->cast_expr.cast_expr);
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(index ");
        print_expr(expr->index_expr.expr);
        printf(" [");
        print_expr(expr->index_expr.index);
        printf("]");
        break;
    case EXPR_FIELD:
        printf("(field ");
        print_expr(expr->field_expr.expr);
        printf(" %s)", expr->field_expr.field);
        break;
    default:
        assert(0);
    }
}

void print_decl(Decl* decl) {
	switch (decl->type) {
	case DECL_ENUM:
		printf("(enum %s {", decl->name);
		for (size_t i = 0; i < decl->enum_decl.num_enum_items; i++) {
			printf("%s=%d, ", decl->enum_decl.enum_items[i].name, decl->enum_decl.enum_items[i].int_val);
		}
		printf("})");
		break;
	case DECL_STRUCT:
	case DECL_UNION:
		printf("(%s %s {", decl->type == DECL_UNION ? "union" : "struct", decl->name);
		for (size_t i = 0; i < decl->aggregate_decl.num_aggregate_items; i++) {
			AggregateItem aggr_item = decl->aggregate_decl.aggregate_items[i];
			print_typespec(aggr_item.type);
			printf(" %s = ", aggr_item.name);
			print_expr(aggr_item.expr);
			printf(", ");
		}
		printf("})");
		break;
	case DECL_CONST:
		printf("(const %s :=", decl->name);
		print_expr(decl->const_decl.expr);
		printf(")");
		break;
	case DECL_VAR:
		printf("(");
		print_typespec(decl->var_decl.type);
		printf(" %s =", decl->name);
		print_expr(decl->var_decl.expr);
		break;
	case DECL_TYPEDEF:
		printf("(typedef %s ", decl->name);
		print_typespec(decl->typedef_decl.type);
		printf(")");
		break;
	case DECL_FUNC:
		printf("(funcdec %s (", decl->name);
		print_typespec(decl->func_decl.ret_type);
		printf(") (");
		for (size_t i = 0; i < decl->func_decl.num_params; i++) {
			FuncParam param = decl->func_decl.params[i];
			print_typespec(param.type);
			printf(" %s, ", param.name);
		}
		printf("))");
		break;
	default:
		assert(0);
	}
}

#pragma TODO(define print statements)

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
