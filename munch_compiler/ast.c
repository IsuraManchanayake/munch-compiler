typedef struct Expr Expr;
typedef struct Stmnt Stmnt;
typedef struct Decl Decl;
typedef struct DeclSet DeclSet;

typedef struct Type Type;

typedef struct TypeSpec TypeSpec;
typedef struct FuncParam FuncParam;

Arena ast_arena;

void* ast_dup(const void* ast, size_t size) {
    if (ast == NULL || size == 0) {
        return NULL;
    }
    void* dup = arena_alloc(&ast_arena, size);
    memcpy(dup, ast, size);
    return dup;
}

#define _ast_dup(x) (ast_dup(x, num_##x * sizeof(*x)))

typedef struct BlockStmnt {
    size_t num_stmnts;
    Stmnt** stmnts;
} BlockStmnt;

// TypeSpecs ==============================================

typedef enum TypeSpecType {
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_PTR
} TypeSpecType;

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
    Type* resolved_type;
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
    typespec->func = (FuncTypeSpec) { .ret_type=ret_type, .num_params=num_params, .params=_ast_dup(params) };
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

typedef struct EnumItem {
    const char* name;
	Expr* expr;
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

struct DeclSet {
    Decl** decls;
    size_t num_decls;
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
	decl->enum_decl = (EnumDecl) { .num_enum_items = num_enum_items, .enum_items = _ast_dup(enum_items) };
	return decl;
}

Decl* decl_aggregate(DeclType type, const char* name, size_t num_aggregate_items, AggregateItem* aggregate_items) {
	assert(type == DECL_UNION || type == DECL_STRUCT);
	Decl* decl = decl_alloc(type, name);
	decl->aggregate_decl = (AggregateDecl) { 
        .num_aggregate_items = num_aggregate_items, 
        .aggregate_items = _ast_dup(aggregate_items) 
    };
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
	decl->func_decl = (FuncDecl) { 
        .num_params = num_params, 
        .params = _ast_dup(params), 
        .ret_type = ret_type, 
        .block = block 
    };
	return decl;
}

DeclSet* declset(Decl** decls, size_t num_decls) {
    DeclSet* declset = arena_alloc(&ast_arena, sizeof(DeclSet));
    memset(declset, 0, sizeof(DeclSet));
    declset->decls = _ast_dup(decls);
    declset->num_decls = num_decls;
    return declset;
}

// ========================================================

// Exprs ==================================================

typedef enum ExprType {
    EXPR_NONE,
    EXPR_TERNARY,
    EXPR_BINARY,
    EXPR_PRE_UNARY,
    EXPR_POST_UNARY,
    EXPR_CALL,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_NAME,
    EXPR_COMPOUND,
    EXPR_CAST,
    EXPR_INDEX,
    EXPR_FIELD,
    EXPR_SIZEOF_TYPE,
    EXPR_SIZEOF_EXPR
} ExprType;

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

typedef enum CompoundItemType {
    COMPOUND_DEFAULT,
    COMPOUND_INDEX,
    COMPOUND_NAME
} CompoundItemType;

typedef struct CompoundItem {
    CompoundItemType type;
    union {
        const char* name;
        Expr* index;
    };
    Expr* value;
} CompoundItem;

typedef struct CompoundExpr {
	TypeSpec* type;
	size_t num_compound_items;
    CompoundItem* compound_items;
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

typedef struct SizeofExpr {
    union {
        TypeSpec* type;
        Expr* expr;
    };
} SizeofExpr;

// --------------------------------------------------------

struct Expr {
    ExprType type;
    Type* resolved_type;
    union {
		TernaryExpr ternary_expr;
		BinaryExpr binary_expr;
		UnaryExpr pre_unary_expr; // pre inc and dec are included
        UnaryExpr post_unary_expr; // only post inc and dec
		CallExpr call_expr;
		IntExpr int_expr;
		FloatExpr float_expr;
		StrExpr str_expr;
		NameExpr name_expr;
		CompoundExpr compound_expr;
		CastExpr cast_expr;
		IndexExpr index_expr;
		FieldExpr field_expr;
        SizeofExpr sizeof_expr;
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

Expr* expr_unary(ExprType unary_type, TokenType op, Expr* uexpr) {
    assert(unary_type == EXPR_PRE_UNARY || unary_type == EXPR_POST_UNARY);
	assert(op == TOKEN_INC || op == TOKEN_DEC || op == '+'
		|| op == '-' || op == '*' || op == '&' || op == '!' || op == '~');
    Expr* expr = expr_alloc(unary_type);
	expr->pre_unary_expr = (UnaryExpr) { .expr=uexpr, .op=op };
    return expr;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right) {
	assert((op >= TOKEN_LOG_AND && op <= TOKEN_GTEQ) || op == '+'
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
	expr->call_expr = (CallExpr) { .expr=call_expr, .num_args=num_args, .args=_ast_dup(args) };
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

CompoundItem compound_default(Expr* value) {
    return (CompoundItem) { .type = COMPOUND_DEFAULT, .value = value };
}

CompoundItem compound_index(Expr* index, Expr* value) {
    return (CompoundItem) { .type = COMPOUND_INDEX, .index = index, .value = value };
}

CompoundItem compound_name(const char* name, Expr* value) {
    return (CompoundItem) { .type = COMPOUND_NAME, .name = name, .value = value };
}

Expr* expr_compound(TypeSpec* type, size_t num_compound_items, CompoundItem* compound_items) {
    Expr* expr = expr_alloc(EXPR_COMPOUND);
    expr->compound_expr = (CompoundExpr) { 
        .type = type, 
        .num_compound_items = num_compound_items, 
        .compound_items = _ast_dup(compound_items) 
    };
    return expr;
}

Expr* expr_sizeof_type(TypeSpec* s_type) {
    Expr* expr = expr_alloc(EXPR_SIZEOF_TYPE);
    expr->sizeof_expr.type = s_type;
    return expr;
}

Expr* expr_sizeof_expr(Expr* s_expr) {
    Expr* expr = expr_alloc(EXPR_SIZEOF_EXPR);
    expr->sizeof_expr.expr = s_expr;
    return expr;
}

// ========================================================

// Statements =============================================

typedef enum StmntType {
    STMNT_NONE,
    STMNT_DECL,
    STMNT_RETURN,
    STMNT_IF_ELSE,
    STMNT_SWITCH,
    STMNT_WHILE,
    STMNT_DO_WHILE,
    STMNT_FOR,
    STMNT_ASSIGN,
    STMNT_INIT,
    STMNT_BREAK,
    STMNT_CONTINUE,
    STMNT_BLOCK,
    STMNT_EXPR
} StmntType;

typedef struct DeclStmnt {
    Decl* decl;
} DeclStmnt;

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
    Stmnt** init;
	Expr* cond;
    size_t num_update;
    Stmnt** update;
	BlockStmnt block;
} ForStmnt;

typedef struct AssignStmnt {
	Expr* left;
	Expr* right;
	TokenType op;
} AssignStmnt;

typedef struct InitStmnt {
    Expr* left;
    Expr* right;
} InitStmnt;

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
        DeclStmnt decl_stmnt;
		IfElseIfStmnt ifelseif_stmnt;
		SwitchStmnt switch_stmnt;
		WhileStmnt while_stmnt;
		ForStmnt for_stmnt;
		AssignStmnt assign_stmnt;
        InitStmnt init_stmnt;
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

Stmnt* stmnt_decl(Decl* decl) {
    Stmnt* stmnt = stmnt_alloc(STMNT_DECL);
    stmnt->decl_stmnt = (DeclStmnt) { decl };
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
		.else_ifs=_ast_dup(else_ifs), 
		.else_block=else_block
	};
	return stmnt;
}

Stmnt* stmnt_switch(Expr* switch_expr, size_t num_case_blocks, CaseBlock* case_blocks, BlockStmnt default_block) {
	Stmnt* stmnt = stmnt_alloc(STMNT_SWITCH);
	stmnt->switch_stmnt = (SwitchStmnt) { 
		.switch_expr = switch_expr, 
		.num_case_blocks = num_case_blocks,
		.case_blocks = _ast_dup(case_blocks),
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

Stmnt* stmnt_for(size_t num_init, Stmnt** init, Expr* cond, size_t num_update, Stmnt** update, BlockStmnt block) {
	Stmnt* stmnt = stmnt_alloc(STMNT_FOR);
	stmnt->for_stmnt = (ForStmnt) { 
        .num_init=num_init,
        .init = _ast_dup(init),
        .cond = cond, 
        .num_update=num_update, 
        .update = _ast_dup(update),
        .block = block 
    };
	return stmnt;
}

Stmnt* stmnt_assign(Expr* left, Expr* right, TokenType op) {
    assert(is_between(op, TOKEN_ADD_ASSIGN, TOKEN_RSHIFT_ASSIGN) || op == '=');
	Stmnt* stmnt = stmnt_alloc(STMNT_ASSIGN);
	stmnt->assign_stmnt = (AssignStmnt) { .left = left, .right = right, .op = op };
	return stmnt;
}

Stmnt* stmnt_init(Expr* left, Expr* right) {
    Stmnt* stmnt = stmnt_alloc(STMNT_INIT);
    stmnt->init_stmnt = (InitStmnt) { .left = left, .right = right };
    return stmnt;
}

Stmnt* stmnt_break(void) {
	return stmnt_alloc(STMNT_BREAK);
}

Stmnt* stmnt_continue(void) {
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

#undef _ast_dup

// ========================================================

// tests ==================================================

// see print.c

// ========================================================
