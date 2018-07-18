typedef struct Expr Expr;
typedef struct Stmnt Stmnt;
typedef struct Decl Decl;

typedef struct TypeSpec TypeSpec;
typedef struct FuncParam FuncParam;

Arena ast_arena;

typedef struct BlockStmnt {
    size_t num_stmnts;
    Stmnt** stmnts;
} BlockStmnt;

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
    TypeSpec* typespec = arena_alloc(&ast_arena, sizeof(TypeSpec));
    memset(typespec, 0, sizeof(TypeSpec));
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
    BlockStmnt block;
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
    Decl* decl = arena_alloc(&ast_arena, sizeof(Decl));
    memset(decl, 0, sizeof(Decl));
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

Decl* decl_func(const char* name, size_t num_params, FuncParam* params, TypeSpec* ret_type, BlockStmnt block) {
	Decl* decl = decl_alloc(DECL_FUNC, name);
	decl->func_decl = (FuncDecl) { .num_params = num_params, .params = params, .ret_type = ret_type, .block = block };
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
    Expr* expr = arena_alloc(&ast_arena, sizeof(Expr));
    memset(expr, 0, sizeof(Expr));
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
    BlockStmnt block;
} ElseIfItem;

typedef struct IfElseIfStmnt {
	Expr* if_cond;
	BlockStmnt then_block;
	size_t num_else_ifs;
	ElseIfItem* else_ifs;
    BlockStmnt else_block;
} IfElseIfStmnt;

typedef struct CaseBlock {
	Expr* case_expr;
    BlockStmnt block;
} CaseBlock;

typedef struct SwitchStmnt {
	Expr* switch_expr;
	size_t num_case_blocks;
	CaseBlock* case_blocks;
    BlockStmnt default_block;
} SwitchStmnt;

typedef struct WhileStmnt {
	Expr* cond;
    BlockStmnt block;
} WhileStmnt;

typedef struct ForStmnt {
    size_t num_init;
	Expr** init;
	Expr* cond;
    size_t num_update;
	Expr** update;
	BlockStmnt block;
} ForStmnt;

typedef struct AssignStmnt {
	Expr* left;
	Expr* right;
	TokenType op;
} AssignStmnt;

typedef struct ReturnStmnt {
	Expr* expr;
} ReturnStmnt;

typedef struct ExprStmnt {
    Expr* expr;
} ExprStmnt;

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
        ExprStmnt expr_stmnt;
    };
};

// --------------------------------------------------------

Stmnt* stmnt_alloc(StmntType type) {
    Stmnt* stmnt = arena_alloc(&ast_arena, sizeof(Stmnt));
    memset(stmnt, 0, sizeof(Stmnt));
	stmnt->type = type;
	return stmnt;
}

Stmnt* stmnt_return(Expr* expr) {
	Stmnt* stmnt = stmnt_alloc(STMNT_RETURN);
	stmnt->return_stmnt = (ReturnStmnt) { expr };
	return stmnt;
}

Stmnt* stmnt_ifelseif(Expr* if_cond, BlockStmnt then_block, size_t num_else_ifs, ElseIfItem* else_ifs, BlockStmnt else_block) {
	Stmnt* stmnt = stmnt_alloc(STMNT_IF_ELSE);
	stmnt->ifelseif_stmnt = (IfElseIfStmnt) {
		.if_cond=if_cond, 
		.then_block= then_block,
		.num_else_ifs=num_else_ifs,
		.else_ifs=else_ifs, 
		.else_block=else_block
	};
	return stmnt;
}

Stmnt* stmnt_switch(Expr* switch_expr, size_t num_case_blocks, CaseBlock* case_blocks, BlockStmnt default_block) {
	Stmnt* stmnt = stmnt_alloc(STMNT_SWITCH);
	stmnt->switch_stmnt = (SwitchStmnt) { 
		.switch_expr = switch_expr, 
		.num_case_blocks = num_case_blocks,
		.case_blocks = case_blocks,
		.default_block = default_block
	};
	return stmnt;
}

Stmnt* stmnt_while(StmntType type, Expr* cond, BlockStmnt block) {
	assert(type == STMNT_WHILE || type == STMNT_DO_WHILE);
	Stmnt* stmnt = stmnt_alloc(type);
	stmnt->while_stmnt = (WhileStmnt) { .cond = cond, .block = block };
	return stmnt;
}

Stmnt* stmnt_for(size_t num_init, Expr** init, Expr* cond, size_t num_update, Expr** update, BlockStmnt block) {
	Stmnt* stmnt = stmnt_alloc(STMNT_FOR);
	stmnt->for_stmnt = (ForStmnt) { .num_init=num_init, .init = init, .cond = cond, .num_update=num_update, .update = update, .block = block };
	return stmnt;
}

Stmnt* stmnt_assign(Expr* left, Expr* right, TokenType op) {
	assert(op == TOKEN_ADD_ASSIGN || op == TOKEN_BIT_AND_ASSIGN || op == TOKEN_BIT_OR_ASSIGN 
		|| op == TOKEN_BIT_XOR_ASSIGN || op == TOKEN_COLON_ASSIGN || op == TOKEN_DIV_ASSIGN  
		|| op == TOKEN_MUL_ASSIGN || op == TOKEN_MOD_ASSIGN || op == TOKEN_LSHIFT_ASSIGN 
		|| op == TOKEN_RSHIFT_ASSIGN || op == TOKEN_ADD_ASSIGN || op == TOKEN_SUB_ASSIGN);
	Stmnt* stmnt = stmnt_alloc(STMNT_ASSIGN);
	stmnt->assign_stmnt = (AssignStmnt) { .left = left, .right = right, .op = op };
	return stmnt;
}

Stmnt* stmnt_break() {
	return stmnt_alloc(STMNT_BREAK);
}

Stmnt* stmnt_continue() {
	return stmnt_alloc(STMNT_CONTINUE);
}

Stmnt* stmnt_block(size_t num_stmnts, Stmnt** stmnts) {
	Stmnt* stmnt = stmnt_alloc(STMNT_BLOCK);
    stmnt->block_stmnt = (BlockStmnt) { .num_stmnts = num_stmnts, .stmnts = stmnts };
	return stmnt;
}

Stmnt* stmnt_expr(Expr* expr) {
    Stmnt* stmnt = stmnt_alloc(STMNT_EXPR);
    stmnt->expr_stmnt = (ExprStmnt) { .expr = expr };
    return stmnt;
}

// ========================================================

// tests ==================================================

// ========================================================
