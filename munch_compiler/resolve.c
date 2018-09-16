typedef struct Type Type;
typedef struct Entity Entity;

typedef enum TypeType {
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PTR,
    TYPE_FUNC,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ARRAY,
    TYPE_ENUM
} TypeType;

typedef struct TypeField {
    Type* type;
    const char* name;
} TypeField;

typedef struct PtrType {
    Type* base;
} PtrType;

typedef struct FuncType {
    size_t num_params;
    Type** params;
    Type* ret;
} FuncType;

typedef struct AggregateType {
    size_t num_fields;
    TypeField* fields;
} AggregateType;

typedef struct ArrayType {
    Type* base;
    size_t size;
} ArrayType;

struct Type {
    TypeType type;
    size_t size;
    Entity* entity;
    union {
        PtrType ptr;
        FuncType func;
        AggregateType aggregate;
        ArrayType array;
    };
};

Type* type_alloc(TypeType type_type) {
    Type* type = xcalloc(1, sizeof(Type));
    type->type = type_type;
    return type;
}

#define VOID_SIZE 0
#define CHAR_SIZE 1
#define INT_SIZE 4
#define FLOAT_SIZE 4
#define PTR_SIZE 8

Type* type_void = &(Type) { .type = TYPE_VOID, .size = VOID_SIZE };
Type* type_char = &(Type) { .type = TYPE_CHAR, .size = CHAR_SIZE };
Type* type_int = &(Type){ .type = TYPE_INT, .size = INT_SIZE };
Type* type_float = &(Type) { .type = TYPE_FLOAT, .size = FLOAT_SIZE };

typedef struct CachedPtrType {
    Type* base;
    Type* ptr;
} CachedPtrType;

typedef struct CachedArrayType {
    Type* base;
    size_t size;
    Type* array;
} CachedArrayType;

typedef struct CachedFuncType {
    size_t num_params;
    Type** params;
    Type* ret;
    Type* func;
} CachedFuncType;

CachedPtrType* cached_ptr_types = NULL;

Type* type_ptr(Type* base) {
    for (CachedPtrType* it = cached_ptr_types; it != buf_end(cached_ptr_types); it++) {
        if (it->base == base) {
            return it->ptr;
        }
    }
    Type* type = type_alloc(TYPE_PTR);
    type->ptr.base = base;
    type->size = PTR_SIZE;
    buf_push(cached_ptr_types, ((CachedPtrType) {.base = base, .ptr = type}));
    return type;
}

CachedArrayType* cached_array_types = NULL;

Type* type_array(Type* base, size_t size) {
    for (CachedArrayType* it = cached_array_types; it != buf_end(cached_array_types); it++) {
        if (it->base == base && it->size == size) {
            return it->array;
        }
    }
    Type* type = type_alloc(TYPE_ARRAY);
    type->array.base = base;
    type->array.size = size;
    type->size = size * base->size;
    buf_push(cached_array_types, ((CachedArrayType){.base = base, .size = size, .array = type }));
    return type;
}

CachedFuncType* cached_func_types = NULL;

Type* type_func(size_t num_params, Type** params, Type* ret) {
    for (CachedFuncType* it = cached_func_types; it != buf_end(cached_func_types); it++) {
        if (it->ret == ret && it->num_params == num_params) {
            bool match = true;
            for (size_t i = 0; i < num_params; i++) {
                if (it->params[i] != params[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return it->func;
            }
        }
    }
    Type* type = type_alloc(TYPE_FUNC);
    type->func.num_params = num_params;
    type->func.params = xcalloc(num_params, sizeof(Type*));
    memcpy(type->func.params, params, num_params * sizeof(Type*));
    type->func.ret = ret;
    type->size = PTR_SIZE;
    buf_push(cached_func_types, ((CachedFuncType) {.num_params = num_params, .params = params, .ret = ret, .func = type }));
    return type;
}

void type_struct(Type* type, size_t num_fields, TypeField* fields) {
    type->type = TYPE_STRUCT;
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++) {
        type->size += it->type->size;
    }
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
}

void type_union(Type* type, size_t num_fields, TypeField* fields) {
    type->type = TYPE_UNION;
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++) {
        type->size = max(type->size, it->type->size);
    }
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
}

Type* type_incomplete(Entity* entity) {
    Type* type = type_alloc(TYPE_INCOMPLETE);
    type->entity = entity;
    return type;
}

typedef enum EntityType {
    ENTITY_NONE,
    ENTITY_VAR,
    ENTITY_CONST,
    ENTITY_FUNC,
    ENTITY_TYPE, // enum, struct, union, typedef
    ENTITY_ENUM_CONST,
    ENTITY_LOCAL,
} EntityType;

typedef enum EntityState {
    ENTITY_STATE_UNRESOVLED,
    ENTITY_STATE_RESOLVING,
    ENTITY_STATE_RESOLVED
} EntityState;

struct Entity {
    EntityType e_type;
    EntityState state;
    const char* name;
    Decl* decl;
    Type* type;
    int64_t value;
};

typedef struct ResolvedExpr {
    int64_t value;
    Type* type;
    bool is_lvalue;
    bool is_const;
} ResolvedExpr;

Entity** global_entities = NULL;
Entity** ordered_entities = NULL;

#define MAX_LOCAL_ENTITIES 1024

Entity* local_entities[MAX_LOCAL_ENTITIES];
Entity** local_entities_end = local_entities;

Entity* get_entity(const char* name) {
    for (Entity** it = local_entities_end; it != local_entities; it--) {
        Entity* local_entity = it[-1];
        if (local_entity->name == name) {
            return local_entity;
        }
    }
    for (size_t i = 0; i < buf_len(global_entities); i++) {
        if (global_entities[i]->name == name) {
            return global_entities[i];
        }
    }
    return NULL;
}

Entity* entity_alloc(EntityType e_type) {
    Entity* entity = xcalloc(1, sizeof(Entity));
    entity->e_type = e_type;
    entity->state = ENTITY_STATE_UNRESOVLED;
    return entity;
}

void push_local_entity(Entity* entity) {
    if (local_entities_end == local_entities + MAX_LOCAL_ENTITIES) {
        fatal("Maximum local entities reached");
    }
    *local_entities_end++ = entity;
}

Entity** enter_scope(void) {
    return local_entities_end;
}

void leave_scope(Entity** local_entity) {
    local_entities_end = local_entity; 
}

Entity* install_decl(Decl* decl) {
    if (get_entity(decl->name)) {
        fatal("%s is already defined", decl->name);
    }
    EntityType e_type;
    switch (decl->type) {
    case DECL_STRUCT:
    case DECL_UNION:
    case DECL_TYPEDEF:
    case DECL_ENUM:
        e_type = ENTITY_TYPE;
        break;
    case DECL_VAR:
        e_type = ENTITY_VAR;
        break;
    case DECL_CONST:
        e_type = ENTITY_CONST;
        break;
    case DECL_FUNC:
        e_type = ENTITY_FUNC;
        break;
    default:
        assert(0);
    }
    Entity* entity = entity_alloc(e_type);
    entity->decl = decl;
    entity->name = decl->name;
    if (decl->type == DECL_STRUCT || decl->type == DECL_UNION) {
        entity->state = ENTITY_STATE_RESOLVED;
        entity->type = type_incomplete(entity);
    }
    else if (decl->type == DECL_ENUM) {
        entity->decl = decl_typedef(decl->name, typespec_name(str_intern("int")));
        for (size_t i = 0; i < decl->enum_decl.num_enum_items; i++) {
            Entity* enum_entity = entity_alloc(ENTITY_ENUM_CONST);
            EnumItem enum_item = decl->enum_decl.enum_items[i];
            enum_entity->name = enum_item.name;
            if (enum_item.expr) {
                enum_entity->decl = decl_const(enum_item.name, enum_item.expr);
            }
            else if (i == 0) {
                enum_entity->decl = decl_const(enum_item.name, expr_int(0));
            }
            else {
                Expr* prev_enum_name = expr_name(decl->enum_decl.enum_items[i - 1].name);
                enum_entity->decl = decl_const(enum_item.name, expr_binary('+', expr_int(1), prev_enum_name));
            }
            enum_entity->type = type_int;
            buf_push(global_entities, enum_entity);
        }
    }
    buf_push(global_entities, entity);
    return entity;
}

Entity* built_in_type(Type* type, const char* name) {
    Entity* entity = entity_alloc(ENTITY_TYPE);
    entity->name = str_intern(name);
    entity->decl = NULL;
    entity->state = ENTITY_STATE_RESOLVED;
    entity->type = type;
    return entity;
}

#define _BUILT_IN_TYPE(t) buf_push(global_entities, built_in_type(type_ ## t, #t));

void install_built_in_types(void) {
    _BUILT_IN_TYPE(void);
    _BUILT_IN_TYPE(char);
    _BUILT_IN_TYPE(int);
    _BUILT_IN_TYPE(float);
}

#undef _BUILT_IN_TYPE

Entity* built_in_const(Type* type, const char* name, Expr* const_expr) {
    Entity* entity = entity_alloc(ENTITY_CONST);
    entity->name = name;
    entity->decl = decl_const(name, const_expr);
    entity->state = ENTITY_STATE_RESOLVED;
    entity->type = type;
    return entity;
}

#define _BUILT_IN_CONST(t, n, v) buf_push(global_entities, built_in_const(type_ ## t, n, expr_ ## t(v)))

void install_built_in_consts(void) {
    _BUILT_IN_CONST(int, kwrd_true, 1);
    _BUILT_IN_CONST(int, kwrd_false, 0);
    _BUILT_IN_CONST(float, kwrd_PI, 3.14159265358);
}

#undef _BUILT_IN_CONST

ResolvedExpr resolve_expr(Expr* expr, Type* expected_type);
void resolve_entity(Entity* entity);

ResolvedExpr resolve_name(const char* name) {
    Entity* entity = get_entity(name);
    if (!entity) {
        fatal("Name %s is not found in declarations", name);
    }
    resolve_entity(entity);
    if (entity->e_type == ENTITY_VAR) {
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = true, .is_const = false };
    }
    else if (entity->e_type == ENTITY_CONST || entity->e_type == ENTITY_ENUM_CONST) {
        return (ResolvedExpr) { .value = entity->value, .type = entity->type, .is_lvalue = true, .is_const = true };
    }
    else if (entity->e_type == ENTITY_FUNC) {
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = false, .is_const = false };
    }
    else if (entity->e_type == ENTITY_LOCAL) {
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = true, .is_const = false };
    }
    else {
        fatal("A value expression is expected by %s", name);
        return (ResolvedExpr) { 0 };
    }
}

ResolvedExpr resolve_binary_expr(Expr* expr);
ResolvedExpr resolve_pre_unary_expr(Expr* expr);
ResolvedExpr resolve_post_unary_expr(Expr* expr);
ResolvedExpr resolve_int_expr(Expr* expr);
ResolvedExpr resolve_float_expr(Expr* expr);
ResolvedExpr resolve_name_expr(Expr* expr);
ResolvedExpr resolve_index_expr(Expr* expr);
ResolvedExpr resolve_field_expr(Expr* expr);
ResolvedExpr resolve_sizeof_expr_expr(Expr* expr);
ResolvedExpr resolve_sizeof_type_expr(Expr* expr);
ResolvedExpr resolve_ternary_expr(Expr* expr, Type* expected_type);
ResolvedExpr resolve_str_expr(Expr* expr);
ResolvedExpr resolve_call_expr(Expr* expr, Type* expected_type);
ResolvedExpr resolve_compound_expr(Expr* expr, Type* expected_type);
ResolvedExpr resolve_cast_expr(Expr* expr);

void complete_type(Type* type);
Type* resolve_typespec(TypeSpec* typespec);

ResolvedExpr resolve_expr(Expr* expr, Type* expected_type) {
    if (expr == NULL && expected_type == type_void) {
        return (ResolvedExpr) { .type = type_void, .is_lvalue = false, .is_const = false };
    }
    ResolvedExpr r_expr;
    switch (expr->type) {
    case EXPR_BINARY:
        r_expr = resolve_binary_expr(expr);
        break;
    case EXPR_PRE_UNARY:
        r_expr = resolve_pre_unary_expr(expr);
        break;
    case EXPR_POST_UNARY:
        r_expr = resolve_post_unary_expr(expr);
        break;
    case EXPR_INT:
        r_expr = resolve_int_expr(expr);
        break;
    case EXPR_FLOAT:
        r_expr = resolve_float_expr(expr);
        break;
    case EXPR_NAME:
        r_expr = resolve_name_expr(expr);
        break;
    case EXPR_INDEX:
        r_expr = resolve_index_expr(expr);
        break;
    case EXPR_FIELD:
        r_expr = resolve_field_expr(expr);
        break;
    case EXPR_SIZEOF_TYPE:
        r_expr = resolve_sizeof_type_expr(expr);
        break;
    case EXPR_TERNARY:
        r_expr = resolve_ternary_expr(expr, expected_type);
        break;
    case EXPR_CALL:
        r_expr = resolve_call_expr(expr, expected_type);
        break;
    case EXPR_STR:
        r_expr = resolve_str_expr(expr);
        break;
    case EXPR_COMPOUND:
        r_expr = resolve_compound_expr(expr, expected_type);
        break;
    case EXPR_CAST:
        r_expr = resolve_cast_expr(expr);
        break;
    default:
        assert(0);
    }
    expr->resolved_type = r_expr.type;
    return r_expr;
}

int64_t eval_const_binary_expr(TokenType op, int64_t left, int64_t right) {
    switch (op)
    {
    case '+':
        return left + right;
    case '-':
        return left - right;
    case '*':
        return left * right;
    case '/':
        if (right == 0) {
            fatal("Zero division error in the const div expr");
        }
        return left / right;
    case '%':
        if (right == 0) {
            fatal("Zero division error in the const mod expr");
        }
        return left % right;
    case '<':
        return left < right;
    case '>':
        return left > right;
    case '&':
        return left & right;
    case '|':
        return left | right;
    case '^':
        return left ^ right;
    case TOKEN_LOG_AND:
        return left && right;
    case TOKEN_LOG_OR:
        return left || right;
    case TOKEN_LSHIFT:
        if (right < 0) {
            fatal("Undefined behavior in the const rshift expr");
        }
        return left << right;
    case TOKEN_RSHIFT:
        if (right < 0) {
            fatal("Undefined behavior in the const rshift expr");
        }
        return left >> right;
    case TOKEN_EQ:
        return left == right;
    case TOKEN_NEQ:
        return left != right;
    case TOKEN_LTEQ:
        return left <= right;
    case TOKEN_GTEQ:
        return left >= right;
    default:
        assert(0);
        return 0;
    }
}

ResolvedExpr resolve_binary_expr(Expr* expr) {
    assert(expr->type == EXPR_BINARY);
    ResolvedExpr left = resolve_expr(expr->binary_expr.left, NULL);
    ResolvedExpr right = resolve_expr(expr->binary_expr.right, NULL);
    if (left.type != right.type) {
        fatal("type mismatch in the binary expression");
    }
    assert(left.type == type_int);
    if (left.is_const && right.is_const) {        
        return (ResolvedExpr) { 
            .value = eval_const_binary_expr(expr->binary_expr.op, left.value, right.value), 
            .type = type_int, 
            .is_lvalue = false, 
            .is_const = true 
        };
    }
    else {
        return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false };
    }
}

ResolvedExpr resolve_pre_unary_expr(Expr* expr) {
    assert(expr->type == EXPR_PRE_UNARY);
    ResolvedExpr base_expr = resolve_expr(expr->pre_unary_expr.expr, NULL);
    switch (expr->pre_unary_expr.op) {
    case TOKEN_INC:
        if (base_expr.is_const) {
            fatal("const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            fatal("an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .value = base_expr.value + 1, .type = base_expr.type, .is_lvalue = true, .is_const = false };
    case TOKEN_DEC:
        if (base_expr.is_const) {
            fatal("const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            fatal("an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .value = base_expr.value - 1, .type = base_expr.type, .is_lvalue = true, .is_const = false };
    case '+':
        return (ResolvedExpr) { .value = +base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const };
    case '-':
        return (ResolvedExpr) { .value = -base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const };
    case '*':
        if (base_expr.type->type != TYPE_PTR) {
            fatal("pointer type is expected in a dereference");
        }
        return (ResolvedExpr) { .type = base_expr.type->ptr.base, .is_lvalue = true, .is_const = false };
    case '&':
        if (!base_expr.is_lvalue) {
            fatal("an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .type = type_ptr(base_expr.type), .is_lvalue = false, .is_const = false };
    case '!':
        return (ResolvedExpr) { .value = !base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const };
    case '~':
        return (ResolvedExpr) { .value = ~base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const };
    default:
        assert(0);
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_post_unary_expr(Expr* expr) {
    assert(expr->type == EXPR_POST_UNARY);
    ResolvedExpr base_expr = resolve_expr(expr->post_unary_expr.expr, NULL);
    switch (expr->post_unary_expr.op) {
    case TOKEN_INC:
        if (base_expr.is_const) {
            fatal("const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            fatal("an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .value = base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = false };
    case TOKEN_DEC:
        if (base_expr.is_const) {
            fatal("const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            fatal("an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .value = base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = false };
    default:
        assert(0);
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_int_expr(Expr* expr) {
    assert(expr->type == EXPR_INT);
    return (ResolvedExpr) { .value = (int64_t)expr->int_expr.int_val, .type = type_int, .is_lvalue = false, .is_const = true };
}

ResolvedExpr resolve_float_expr(Expr* expr) {
    assert(expr->type == EXPR_FLOAT);
    return (ResolvedExpr) { .value = *(int64_t*)&(expr->float_expr.float_val), .type = type_float, .is_lvalue = false, .is_const = true };
}

ResolvedExpr resolve_name_expr(Expr* expr) {
    assert(expr->type == EXPR_NAME);
    return resolve_name(expr->name_expr.name);
}

ResolvedExpr resolve_index_expr(Expr* expr) {
    assert(expr->type == EXPR_INDEX);
    ResolvedExpr base_expr = resolve_expr(expr->index_expr.expr, NULL);
    ResolvedExpr index_expr = resolve_expr(expr->index_expr.index, NULL);
    if (base_expr.type->type != TYPE_PTR && base_expr.type->type != TYPE_ARRAY) {
        fatal("An array or ptr is expected as the operand of an array index");
    }
    if (index_expr.type != type_int) {
        fatal("An integer is expected as array index");
    }
    Type* type = base_expr.type->type == TYPE_ARRAY ? base_expr.type->array.base : base_expr.type->ptr.base;
    return (ResolvedExpr) { .type = type, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const };
}

ResolvedExpr resolve_field_expr(Expr* expr) {
    assert(expr->type == EXPR_FIELD);
    ResolvedExpr base_expr = resolve_expr(expr->field_expr.expr, NULL);
    for (size_t i = 0; i < base_expr.type->aggregate.num_fields; i++) {
        if (expr->field_expr.field == base_expr.type->aggregate.fields[i].name) {
            Type* type = base_expr.type->aggregate.fields[i].type;
            complete_type(type);
            return (ResolvedExpr) { .type = type, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const };
        }
    }
    fatal("%s is not a field of %s", expr->field_expr.field, base_expr.type->entity->name);
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_sizeof_expr_expr(Expr* expr) {
    assert(expr->type == EXPR_SIZEOF_EXPR);
    ResolvedExpr base_expr = resolve_expr(expr->sizeof_expr.expr, NULL);
    return (ResolvedExpr) { .value = base_expr.type->size, .type = type_int, .is_lvalue = false, .is_const = true };
}

ResolvedExpr resolve_sizeof_type_expr(Expr* expr) {
    assert(expr->type == EXPR_SIZEOF_TYPE);
    Type* type = resolve_typespec(expr->sizeof_expr.type);
    complete_type(type);
    return (ResolvedExpr) { .value = type->size, .type = type_int, .is_lvalue = false, .is_const = true };
}

ResolvedExpr resolve_ternary_expr(Expr* expr, Type* expected_type) {
    assert(expr->type == EXPR_TERNARY);
    ResolvedExpr cond_expr = resolve_expr(expr->ternary_expr.cond, NULL);
    ResolvedExpr left_expr = resolve_expr(expr->ternary_expr.left, expected_type);
    ResolvedExpr right_expr = resolve_expr(expr->ternary_expr.right, expected_type);
    if (cond_expr.type != type_int) {
        fatal("An integer type is expected in a condition expr in the ternary expr");
    }
    if (expected_type && left_expr.type != expected_type) {
        fatal("The left expr type mismatch with the expected type");
    }
    if (expected_type && right_expr.type != expected_type) {
        fatal("The right expr type mismatch with the expected type");
    }
    if (cond_expr.is_const && left_expr.is_const && right_expr.is_const) {
        return cond_expr.value ? left_expr : right_expr;
    }
    else {
        return (ResolvedExpr) { .type = left_expr.type, .is_lvalue = left_expr.is_lvalue && right_expr.is_lvalue, .is_const = false };
    }
}

ResolvedExpr resolve_str_expr(Expr* expr) {
    assert(expr->type == EXPR_STR);
    return (ResolvedExpr) { .type = type_ptr(type_char), .is_lvalue = false, .is_const = true };
}

ResolvedExpr resolve_call_expr(Expr* expr, Type* expected_type) {
    assert(expr->type == EXPR_CALL);
    ResolvedExpr func_expr = resolve_expr(expr->call_expr.expr, NULL);
    if (expr->call_expr.num_args != func_expr.type->func.num_params) {
        fatal("Argument count in the call expr mismatch with the function type");
    }
    for (size_t i = 0; i < expr->call_expr.num_args; i++) {
        ResolvedExpr param_expr = resolve_expr(expr->call_expr.args[i], func_expr.type->func.params[i]);
        if (param_expr.type != func_expr.type->func.params[i]) {
            fatal("Argument in the call expression mismatch with the function argument type");
        }
    }
    if (expected_type && func_expr.type->func.ret != expected_type) {
        fatal("Function return type mismatch with the expected type");
    }
    return (ResolvedExpr) { .type = func_expr.type->func.ret, .is_lvalue = false, .is_const = false };
}

ResolvedExpr resolve_compound_expr(Expr* expr, Type* expected_type) {
    assert(expr->type == EXPR_COMPOUND);
    TypeSpec* compound_typespec = expr->compound_expr.type;
    if (!expected_type && !compound_typespec) {
        fatal("Implicit types for compound expresssions are not allowed");
    }
    Type* compound_type = expected_type;
    if (expected_type && compound_typespec) {
        if (resolve_typespec(compound_typespec) != expected_type) {
            fatal("Explicit type of the compound expression mismatch with the expected type");
        }
    }
    else if (compound_typespec) {
        compound_type = resolve_typespec(compound_typespec);
    }
    complete_type(compound_type);
    if (compound_type->type == TYPE_STRUCT || compound_type->type == TYPE_UNION) {
        if (compound_type->aggregate.num_fields < expr->compound_expr.num_compound_items) {
            fatal("Number of fields in the compound expression exceeds the aggregate definition field count");
        }
        bool is_const = true;
        for (size_t i = 0, k = 0; i < expr->compound_expr.num_compound_items; i++, k++) {
            CompoundItem compound_item = expr->compound_expr.compound_items[i];
            if (compound_item.type == COMPOUND_INDEX) {
                fatal("Index items are not allowed in aggregate compound expressions");
            }
            if (compound_item.type == COMPOUND_NAME) {
                for (size_t j = 0; j < compound_type->aggregate.num_fields; j++) {
                    if (compound_item.name == compound_type->aggregate.fields[j].name) {
                        k = j;
                        break;
                    }
                }
            }
            if (k >= compound_type->aggregate.num_fields) {
                fatal("Aggregate compound expresssion exceeds expr count");
            }
            ResolvedExpr r_expr = resolve_expr(compound_item.value, compound_type->aggregate.fields[k].type);
            is_const &= r_expr.is_const;
            if (compound_type->aggregate.fields[i].type != r_expr.type) {
                fatal("Field type mismatch with the aggregate definition");
            }
        }
        return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = is_const };
    }
    else if (compound_type->type == TYPE_ARRAY) {
        if (compound_type->array.size < expr->compound_expr.num_compound_items) {
            fatal("Number of fields in the compound expression exceeds the array size");
        }
        bool is_const = true;
        for (size_t i = 0, k = 0; i < expr->compound_expr.num_compound_items; i++, k++) {
            CompoundItem compound_item = expr->compound_expr.compound_items[i];
            if (compound_item.type == COMPOUND_NAME) {
                fatal("Named items are not allowed in array compound expressions");
            }
            if (compound_item.type == COMPOUND_INDEX) {
                ResolvedExpr index_expr = resolve_expr(compound_item.index, NULL);
                if (index_expr.type != type_int && index_expr.type != type_char) {
                    fatal("An int or char expr is expected in indices of array compound expressions");
                }
                if (!index_expr.is_const) {
                    fatal("Const indices are expected in array compound expressions");
                }
                if (index_expr.value < 0) {
                    fatal("Index expr should be non-negative");
                }
                k = (size_t) index_expr.value;
            }
            if (k >= compound_type->array.size) {
                fatal("Index exceeds the array length in the array compound expression");
            }
            ResolvedExpr r_expr = resolve_expr(expr->compound_expr.compound_items[i].value, compound_type->array.base);
            is_const &= r_expr.is_const;
            if (compound_type->array.base != r_expr.type) {
                fatal("Field type mismatch with the array type");
            }
        }
        return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = is_const };
    }
    else {
        fatal("An array or struct or union type is expected in a compound expression");
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_cast_expr(Expr* expr) {
    assert(expr->type == EXPR_CAST);
    Type* cast_type = resolve_typespec(expr->cast_expr.cast_type);
    ResolvedExpr cast_expr = resolve_expr(expr->cast_expr.cast_expr, NULL);
    if (cast_type->type == TYPE_PTR) {
        if (cast_expr.type->type != TYPE_PTR && cast_expr.type->type != TYPE_INT) {
            fatal("Unsupported cast to a pointer");
        }
    }
    else if (cast_type->type == TYPE_INT) {
        if (cast_expr.type->type == TYPE_PTR 
            && cast_expr.type->type != TYPE_INT 
            && cast_expr.type->type != TYPE_FLOAT 
            && cast_expr.type->type == TYPE_CHAR) {
            fatal("Unsupported cast to an int");
        }
    }
    else {
        fatal("Unsupported cast");
    }
    return (ResolvedExpr) { .type = cast_type, .is_lvalue = false, .is_const = false };
}

Type* resolve_typespec_name(TypeSpec* typespec);
Type* resolve_typespec_func(TypeSpec* typespec);
Type* resolve_typespec_array(TypeSpec* typespec);
Type* resolve_typespec_ptr(TypeSpec* typespec);

Type* resolve_typespec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME:
        return resolve_typespec_name(typespec);
    case TYPESPEC_FUNC:
        return resolve_typespec_func(typespec);
    case TYPESPEC_ARRAY:
        return resolve_typespec_array(typespec);
    case TYPESPEC_PTR:
        return resolve_typespec_ptr(typespec);
    default:
        assert(0);
    }
    return NULL;
}

Type* resolve_typespec_name(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_NAME);
    const char* name = typespec->name.name;
    Entity* entity = get_entity(name);
    resolve_entity(entity);
    if (entity->e_type != ENTITY_TYPE) {
        fatal("A type name is expected. Got %s", name);
    }
    return entity->type;
}

Type* resolve_typespec_func(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_FUNC);
    Type** params = xcalloc(typespec->func.num_params, sizeof(Type*));
    for (size_t i = 0; i < typespec->func.num_params; i++) {
        params[i] = resolve_typespec(typespec->func.params[i]);
    }
    Type* ret_type = resolve_typespec(typespec->func.ret_type);
    return type_func(typespec->func.num_params, params, ret_type);
}

Type* resolve_typespec_array(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_ARRAY);
    ResolvedExpr size_expr = resolve_expr(typespec->array.size, NULL);
    if (!size_expr.is_const) {
        fatal("const is expected in an array size expression");
    }
    if (!is_between(size_expr.value, 0, SIZE_MAX)) {
        fatal("invalid expr value for array size");
    }
    return type_array(resolve_typespec(typespec->array.base), (size_t)size_expr.value);
}

Type* resolve_typespec_ptr(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_PTR);
    return type_ptr(resolve_typespec(typespec->ptr.base));
}

void complete_type(Type* type) {
    if (type->type == TYPE_COMPLETING) {
        fatal("Cyclic dependancy in %s", type->entity->name);
    }
    else if (type->type == TYPE_INCOMPLETE) {
        type->type = TYPE_COMPLETING;
        Decl* decl = type->entity->decl;
        assert(decl->type == DECL_STRUCT || decl->type == DECL_UNION);
        TypeField* aggregate_fields = NULL;
        AggregateDecl aggregate_decl = decl->aggregate_decl;
        for (size_t i = 0; i < aggregate_decl.num_aggregate_items; i++) {
            Type* aggregate_type = resolve_typespec(aggregate_decl.aggregate_items[i].type);
            complete_type(aggregate_type);
            if (aggregate_decl.aggregate_items[i].expr) {
                ResolvedExpr aggregate_expr = resolve_expr(aggregate_decl.aggregate_items[i].expr, NULL);
                if (aggregate_type != aggregate_expr.type) {
                    fatal("aggregate type and expression inferred type mismatch");
                }
            }
            buf_push(aggregate_fields, ((TypeField) {.type = aggregate_type, .name = aggregate_decl.aggregate_items[i].name }));
        }
        if (decl->type == DECL_STRUCT) {
            type_struct(type, aggregate_decl.num_aggregate_items, aggregate_fields);
        }
        else if (decl->type == DECL_UNION) {
            type_union(type, aggregate_decl.num_aggregate_items, aggregate_fields);
        }
        else {
            assert(0);
        }
        buf_push(ordered_entities, type->entity);
    }
}

void resolve_entity_type(Entity* entity);
void resolve_entity_var(Entity* entity);
void resolve_entity_const(Entity* entity);
void resolve_entity_enum_const(Entity* entity);
void resolve_entity_func(Entity* entity);

void resolve_entity(Entity* entity) {
    if (entity->state == ENTITY_STATE_RESOLVING) {
        fatal("Cyclic dependancy for %s", entity->name);
    }
    else if (entity->state == ENTITY_STATE_UNRESOVLED) {
        entity->state = ENTITY_STATE_RESOLVING;
        switch (entity->e_type) {
        case ENTITY_TYPE:
            resolve_entity_type(entity);
            break;
        case ENTITY_VAR:
            resolve_entity_var(entity);
            break;
        case ENTITY_ENUM_CONST:
            resolve_entity_enum_const(entity);
            break;
        case ENTITY_CONST:
            resolve_entity_const(entity);
            break;
        case ENTITY_FUNC:
            resolve_entity_func(entity);
            break;
        default:
            assert(0);
        }
        entity->state = ENTITY_STATE_RESOLVED;
    }
}

void resolve_entity_type(Entity* entity) {
    Type* type = resolve_typespec(entity->decl->typedef_decl.type);
    entity->type = type;
    complete_type(type);
    buf_push(ordered_entities, entity);
}

void resolve_entity_var(Entity* entity) {
    Type* type = NULL;
    if (entity->decl->var_decl.type) {
        type = resolve_typespec(entity->decl->var_decl.type);
        entity->type = type;
    }
    if (entity->decl->var_decl.expr) {
        ResolvedExpr r_expr = resolve_expr(entity->decl->var_decl.expr, type);
        if (type && type != r_expr.type) {
            fatal("declared type and the expression types mismatch in %s", entity->name);
        }
        if (r_expr.is_const) {
            entity->value = r_expr.value;
        }
        type = r_expr.type;
        entity->type = type;
    }
    complete_type(type);
    buf_push(ordered_entities, entity);
}

void resolve_entity_const(Entity* entity) {
    ResolvedExpr r_expr = resolve_expr(entity->decl->const_decl.expr, NULL);
    if (!r_expr.is_const) {
        fatal("A const expr is expected in a const declaration");
    }
    entity->type = r_expr.type;
    entity->value = r_expr.value;
    buf_push(ordered_entities, entity);
}

void resolve_entity_enum_const(Entity* entity) {
    resolve_entity_const(entity);
}

void resolve_stmnt_block(BlockStmnt block, Type* ret_type);

void resolve_entity_func(Entity* entity) {
    Type ** param_types = NULL;
    for (size_t i = 0; i < entity->decl->func_decl.num_params; i++) {
        Type* param_type = resolve_typespec(entity->decl->func_decl.params[i].type);
        complete_type(param_type);
        buf_push(param_types, param_type);
    }
    Type* ret_type = NULL;
    if (entity->decl->func_decl.ret_type) {
        ret_type = resolve_typespec(entity->decl->func_decl.ret_type);
        complete_type(ret_type);
    }
    entity->type = type_func(buf_len(param_types), param_types, ret_type);
    buf_push(ordered_entities, entity);
}

Entity* entity_local_var(const char* name, Type* type) {
    Entity* entity = entity_alloc(ENTITY_LOCAL);
    entity->name = name;
    entity->type = type;
    entity->state = ENTITY_STATE_RESOLVED;
    return entity;
}

void resolve_cond_expr(Expr* cond_expr, BlockStmnt block, Type* ret_type) {
    ResolvedExpr cond = resolve_expr(cond_expr, type_int);
    if (cond.type != type_int) {
        fatal("int type is expected in an if condition expr");
    }
    resolve_stmnt_block(block, ret_type);
}

void resolve_stmnt(Stmnt* stmnt, Type* ret_type) {
    switch (stmnt->type) {
    case STMNT_DECL: {
        assert(stmnt->decl_stmnt.decl->type == DECL_VAR);
        push_local_entity(entity_local_var(stmnt->decl_stmnt.decl->name, resolve_typespec(stmnt->decl_stmnt.decl->var_decl.type)));
        break;
    }
    case STMNT_RETURN: { 
        ResolvedExpr return_expr = resolve_expr(stmnt->return_stmnt.expr, ret_type);
        if (return_expr.type != ret_type) {
            fatal("Return statement type mismatch with the function return type");
        }
        break;
    }
    case STMNT_IF_ELSE: {
        resolve_cond_expr(stmnt->ifelseif_stmnt.if_cond, stmnt->ifelseif_stmnt.then_block, ret_type);
        for (size_t i = 0; i < stmnt->ifelseif_stmnt.num_else_ifs; i++) {
            resolve_cond_expr(stmnt->ifelseif_stmnt.else_ifs[i].cond, stmnt->ifelseif_stmnt.else_ifs[i].block, ret_type);
        }
        resolve_stmnt_block(stmnt->ifelseif_stmnt.else_block, ret_type);
        break;
    }
    case STMNT_SWITCH: {
        ResolvedExpr switch_expr = resolve_expr(stmnt->switch_stmnt.switch_expr, NULL);
        for (size_t i = 0; i < stmnt->switch_stmnt.num_case_blocks; i++) {
            ResolvedExpr cond = resolve_expr(stmnt->switch_stmnt.case_blocks[i].case_expr, switch_expr.type);
            if (cond.type != switch_expr.type) {
                fatal("switch type is expected in a case condition expr");
            }
            resolve_stmnt_block(stmnt->switch_stmnt.case_blocks[i].block, ret_type);
        }
        resolve_stmnt_block(stmnt->switch_stmnt.default_block, ret_type);
        break;
    }
    case STMNT_WHILE:
    case STMNT_DO_WHILE: {
        resolve_cond_expr(stmnt->while_stmnt.cond, stmnt->while_stmnt.block, ret_type);
        break;
    }
    case STMNT_FOR: {
        Entity** local_entity = enter_scope();
        for (size_t i = 0; i < stmnt->for_stmnt.num_init; i++) {
            resolve_stmnt(stmnt->for_stmnt.init[i], NULL);
        }
        ResolvedExpr cond = resolve_expr(stmnt->for_stmnt.cond, type_int);
        if (cond.type != type_int) {
            fatal("An int is expected in the condition of a for stmnt");
        }
        for (size_t i = 0; i < stmnt->for_stmnt.num_update; i++) {
            resolve_stmnt(stmnt->for_stmnt.update[i], NULL);
        }
        resolve_stmnt_block(stmnt->for_stmnt.block, ret_type);
        leave_scope(local_entity);
        break;
    }
    case STMNT_ASSIGN: {
        ResolvedExpr left = resolve_expr(stmnt->assign_stmnt.left, NULL);
        ResolvedExpr right = resolve_expr(stmnt->assign_stmnt.right, left.type);
        if (stmnt->assign_stmnt.op != '=') {
            if (left.type != type_int) {
                fatal("An int is expected in the left operand of an assignment");
            }
            if (right.type != type_int) {
                fatal("An int is expected in the right operand of an assignment");
            }
        }
        else {
            if (left.type != right.type) {
                fatal("Left operand and right operand types should be matched in an assignment");
            }
        }
        if (!left.is_lvalue || left.is_const) {
            fatal("A modifiable lvalue is expected as the left operand of an assignement");
        }
        break;
    }
    case STMNT_BREAK:
    case STMNT_CONTINUE:
        break;
    case STMNT_BLOCK: {
        resolve_stmnt_block(stmnt->block_stmnt, ret_type);
        break;
    }
    case STMNT_EXPR: {
        resolve_expr(stmnt->expr_stmnt.expr, NULL);
        break;
    }
    default:
        assert(0);
    }
}

void resolve_stmnt_block(BlockStmnt block, Type* ret_type) {
    Entity** local_sym = enter_scope();
    for (size_t i = 0; i < block.num_stmnts; i++) {
        resolve_stmnt(block.stmnts[i], ret_type);
    }
    leave_scope(local_sym);
}

void resolve_func(Entity* entity) {
    assert(entity->e_type == ENTITY_FUNC);
    Decl* decl = entity->decl;
    for (size_t i = 0; i < decl->func_decl.num_params; i++) {
        Entity* param = entity_local_var(decl->func_decl.params[i].name, resolve_typespec(decl->func_decl.params[i].type));
        push_local_entity(param);
    }
    resolve_stmnt_block(decl->func_decl.block, resolve_typespec(decl->func_decl.ret_type));
}

void complete_entity(Entity* entity) {
    resolve_entity(entity);
    if (entity->e_type == ENTITY_TYPE) {
        complete_type(entity->type);
    }
    else if (entity->e_type == ENTITY_FUNC) {
        resolve_func(entity);
    }
}

void complete_entities(void) {
    for (Entity** it = global_entities; it != buf_end(global_entities); it++) {
        complete_entity(*it);
    }
}

#pragma TODO("Align fields in aggregate types")
#pragma TODO("Do pointer decay for arrays")

void resolve_decl_test(void) {
    install_built_in_types();
    install_built_in_consts();
    const char* src[] = {
        "struct V {x: int; y: int;}",
        "var v: V = {y=1, x=2}",
        "var w: int[3] = {1, [2]=3}",
        "var vv: V[2] = {{1, 2}, {3, 4}}",
        "func add(a: V, b: V): V { var c: V; c = {a.x + b.x, a.y + b.y}; return c; }",
        "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}",
        "func printf(a: int) {return;}",
        "func one(n: int) { if(n < 0) {return;} var i:int; for(i = 0; i < n; i++) {printf(1);}}",
        //"struct PP {x:int; y:int;}",
        //"const a = (3 + 3 * 3 / 3) << 3",
        //"const b = 32 % (~32 + 1 == -32)",
        //"var c = cast(void*) &a",
        //"var d = cast(int*) c",
        //"var e = *d",
        //"var ppa = &a",
        //"var v = (:PP){1, Q}",
        //"enum A{P, Q=5+3*3, R, S}",
        //"union AA {x:int; y:int*;}",
        //"var z = AA {1, ppa}",
        //"var wer = (:int[3]){1, 3, 4}",
        //"const a = b",
        //"const b = 3",
        //"var _p = PP{1, 2}",
        //"var _q = QQ{1, 3}",
        //"var pp: PP = f(_p, _q)",
        //"func f(a: PP, b: QQ): PP { return PP{a.x + b.x + true, a.y + b.y + false}; }",
        //"var g: func (PP, QQ):PP = f",
        //"struct QQ {x: int; y: int;}",
        //"var aa = wer[1]",
        //"var paa = &aa",
        //"var gd = paa[2]",
        /**/
        //"var c = &a",
        //"struct V { foo : int[3] = d; k: int = *c; }",
        //"var d: A[3]",
        //"const n = 1+sizeof(p)",
        //"var p: T*",
        //"var u = *p",
        //"struct T { a: int[n+S]; }",
        //"var r = &t.a",
        //"var t: T",
        //"typedef S_ = int[n+m]",
        //"const m = sizeof(t.a)",
        //"var i = n+m",
        //"var q = &i",
    };
    for (int i = 0; i < sizeof(src) / sizeof(*src); i++) {
        init_stream(src[i]);
        Decl* decl = parse_decl();
        install_decl(decl);
    }
    complete_entities();
    for (Entity** it = ordered_entities; it != buf_end(ordered_entities); it++) {
        print_decl((*it)->decl);
        printf("\n");
    }
}

resolve_test(void) {
    printf("----- gen.c -----\n");
    resolve_decl_test();
    printf("gen test passed");
}