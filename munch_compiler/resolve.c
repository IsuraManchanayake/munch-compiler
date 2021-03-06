typedef struct Type Type;
typedef struct Entity Entity;

bool enable_warnings = true;

#define resolve_error(loc, fmt, ...) \
    do { \
        printf("RESOLVE_ERROR(%s:%zu) ", (loc).src_name ? (loc).src_name : "", (loc).line_num); \
        fatal(fmt, ##__VA_ARGS__); \
    } while(0)

#define resolve_warning(loc, fmt, ...) \
    do { \
        printf("RESOLVE_WARNING(%s:%zu) ", (loc).src_name ? (loc).src_name : "", (loc).line_num); \
        warning(fmt, ##__VA_ARGS__); \
    } while(0)

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

Map cached_ptr_types;

Type* type_ptr(Type* base) {
    Type* ptr_type = map_get(&cached_ptr_types, base);
    if (ptr_type) {
        return ptr_type;
    }
    Type* type = type_alloc(TYPE_PTR);
    type->ptr.base = base;
    type->size = PTR_SIZE;
    map_put(&cached_ptr_types, base, type);
    return type;
}

Map cached_array_types;

Type* type_array(Type* base, size_t size) {
    Type* array_type = map_get_ptr_uint(&cached_array_types, (void*)base, size);
    if (array_type) {
        return array_type;
    }
    Type* type = type_alloc(TYPE_ARRAY);
    type->array.base = base;
    type->array.size = size;
    type->size = size * base->size;
    map_put_ptr_uint(&cached_array_types, (void*)base, size, type);
    return type;
}

Map cached_func_types;

Type* type_func(size_t num_params, Type** params, Type* ret) {
    Type* func_type = map_get_key_list(&cached_func_types, (void**)params, num_params, ret);
    if (func_type) {
        return func_type;
    }
    Type* type = type_alloc(TYPE_FUNC);
    type->func.num_params = num_params;
    type->func.params = xcalloc(num_params, sizeof(Type*));
    memcpy(type->func.params, params, num_params * sizeof(Type*));
    type->func.ret = ret;
    type->size = PTR_SIZE;
    map_put_key_list(&cached_func_types, (void**)params, num_params, ret, type);
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
    bool is_folded;
    bool is_set;
    bool is_used;
    SrcLoc loc;
};

typedef struct ResolvedExpr {
    int64_t value;
    Type* type;
    bool is_lvalue;
    bool is_const;
    bool is_folded;
} ResolvedExpr;

Map global_entities = { 0 };
Entity** ordered_entities = NULL;

#define MAX_LOCAL_ENTITIES (1 << 20)

Entity* local_entities[MAX_LOCAL_ENTITIES];
Entity** local_entities_end = local_entities;

Entity* get_entity(const char* name) {
    size_t iter = 0;
    for (Entity** it = local_entities_end; it != local_entities; it--, iter++) {
        Entity* local_entity = it[-1];
        if (local_entity->name == name) {
            return local_entity;
        }
    }
    return map_get(&global_entities, (char*)name);
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
        resolve_error(entity->loc, "Maximum local entities reached");
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
        resolve_error(decl->loc, "%s is already defined", decl->name);
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
    entity->loc = decl->loc;
    if (decl->type == DECL_STRUCT || decl->type == DECL_UNION) {
        entity->state = ENTITY_STATE_RESOLVED;
        entity->type = type_incomplete(entity);
    }
    else if (decl->type == DECL_ENUM) {
        entity->decl = decl_typedef(decl->name, typespec_name(str_intern("int")));
        for (size_t i = 0; i < decl->enum_decl.num_enum_items; i++) {
            Entity* enum_entity = entity_alloc(ENTITY_ENUM_CONST);
            enum_entity->loc = decl->loc;
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
            map_put(&global_entities, (char*)enum_entity->name, enum_entity);
        }
    }
    map_put(&global_entities, (char*)entity->name, entity);
    return entity;
}

void install_decls(DeclSet* declset) {
    for (size_t i = 0; i < declset->num_decls; i++) {
        install_decl(declset->decls[i]);
    }
}

Entity* built_in_type(Type* type, const char* name) {
    Entity* entity = entity_alloc(ENTITY_TYPE);
    entity->name = str_intern(name);
    entity->decl = NULL;
    entity->state = ENTITY_STATE_RESOLVED;
    entity->type = type;
    entity->loc = (SrcLoc) { "{built-in}", 0 };
    entity->is_set = true;
    entity->is_used = true;
    return entity;
}

#define _BUILT_IN_TYPE(t) \
    do { \
        map_put(&global_entities, (char*)str_intern(#t), built_in_type(type_ ## t, #t)); \
    } while (0)

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
    entity->loc = (SrcLoc) { "{built-in}", 0 };
    entity->is_set = true;
    entity->is_used = true;
    return entity;
}

#define _BUILT_IN_CONST(t, n, v) \
    do { \
        map_put(&global_entities, (char*)n, (Entity*)built_in_const(type_ ## t, n, expr_## t(v))); \
    } while(0)

void install_built_in_consts(void) {
    _BUILT_IN_CONST(int, kwrd_true, 1);
    _BUILT_IN_CONST(int, kwrd_false, 0);
    _BUILT_IN_CONST(float, kwrd_PI, 3.14159265358);
}

#undef _BUILT_IN_CONST

ResolvedExpr resolve_expr(Expr* expr, Type* expected_type, bool is_global);
void resolve_entity(Entity* entity);

ResolvedExpr resolve_name(const char* name, bool is_global) {
    Entity* entity = get_entity(name);
    if (!entity) {
        fatal("Name %s is not found in declarations", name);
    }
    resolve_entity(entity);
    entity->is_used = true;
    if (entity->e_type == ENTITY_VAR) {
        if (is_global) {
            return (ResolvedExpr) { .value = entity->value, .type = entity->type, .is_lvalue = true, .is_const = false, .is_folded = entity->is_folded };
        }
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = true, .is_const = false, .is_folded = false };
    }
    else if (entity->e_type == ENTITY_CONST || entity->e_type == ENTITY_ENUM_CONST) {
        return (ResolvedExpr) { .value = entity->value, .type = entity->type, .is_lvalue = false, .is_const = true, .is_folded = true };
    }
    else if (entity->e_type == ENTITY_FUNC) {
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = false, .is_const = false, .is_folded = false };
    }
    else if (entity->e_type == ENTITY_LOCAL) {
        return (ResolvedExpr) { .type = entity->type, .is_lvalue = true, .is_const = false, .is_folded = false };
    }
    else {
        resolve_error(entity->loc, "A value expression is expected by %s", name);
        return (ResolvedExpr) { 0 };
    }
}

ResolvedExpr resolve_binary_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_pre_unary_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_post_unary_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_int_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_float_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_name_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_index_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_field_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_sizeof_expr_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_sizeof_type_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_ternary_expr(Expr* expr, Type* expected_type, bool is_global);
ResolvedExpr resolve_str_expr(Expr* expr, bool is_global);
ResolvedExpr resolve_call_expr(Expr* expr, Type* expected_type, bool is_global);
ResolvedExpr resolve_compound_expr(Expr* expr, Type* expected_type, bool is_global);
ResolvedExpr resolve_cast_expr(Expr* expr, bool is_global);

void complete_type(Type* type);
Type* resolve_typespec(TypeSpec* typespec);

ResolvedExpr resolve_expr(Expr* expr, Type* expected_type, bool is_global) {
    if (expr == NULL && expected_type == type_void) {
        return (ResolvedExpr) { .type = type_void, .is_lvalue = false, .is_const = false, .is_folded = false };
    }
    ResolvedExpr r_expr;
    switch (expr->type) {
    case EXPR_BINARY:
        r_expr = resolve_binary_expr(expr, is_global);
        break;
    case EXPR_PRE_UNARY:
        r_expr = resolve_pre_unary_expr(expr, is_global);
        break;
    case EXPR_POST_UNARY:
        r_expr = resolve_post_unary_expr(expr, is_global);
        break;
    case EXPR_INT:
        r_expr = resolve_int_expr(expr, is_global);
        break;
    case EXPR_FLOAT:
        r_expr = resolve_float_expr(expr, is_global);
        break;
    case EXPR_NAME:
        r_expr = resolve_name_expr(expr, is_global);
        break;
    case EXPR_INDEX:
        r_expr = resolve_index_expr(expr, is_global);
        break;
    case EXPR_FIELD:
        r_expr = resolve_field_expr(expr, is_global);
        break;
    case EXPR_SIZEOF_TYPE:
        r_expr = resolve_sizeof_type_expr(expr, is_global);
        break;
    case EXPR_SIZEOF_EXPR:
        r_expr = resolve_sizeof_expr_expr(expr, is_global);
        break;
    case EXPR_TERNARY:
        r_expr = resolve_ternary_expr(expr, expected_type, is_global);
        break;
    case EXPR_CALL:
        r_expr = resolve_call_expr(expr, expected_type, is_global);
        break;
    case EXPR_STR:
        r_expr = resolve_str_expr(expr, is_global);
        break;
    case EXPR_COMPOUND:
        r_expr = resolve_compound_expr(expr, expected_type, is_global);
        break;
    case EXPR_CAST:
        r_expr = resolve_cast_expr(expr, is_global);
        break;
    default:
        assert(0);
    }
    expr->resolved_type = r_expr.type;
    if (r_expr.type == type_int && r_expr.is_folded) {
        expr->folded_value = r_expr.value;
        expr->is_folded = r_expr.is_folded;
    }
    return r_expr;
}

Entity* get_expr_entity(Expr* expr) {
    if (expr->type == EXPR_NAME) {
        return get_entity(expr->name_expr.name);
    }
    return NULL;
}

int64_t eval_const_binary_expr(TokenType op, int64_t left, int64_t right, SrcLoc loc) {
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
            resolve_error(loc, "Zero division error in the const div expr");
        }
        return left / right;
    case '%':
        if (right == 0) {
            resolve_error(loc, "Zero division error in the const mod expr");
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
            resolve_error(loc, "Undefined behavior in the const rshift expr");
        }
        return left << right;
    case TOKEN_RSHIFT:
        if (right < 0) {
            resolve_error(loc, "Undefined behavior in the const rshift expr");
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

ResolvedExpr resolve_binary_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_BINARY);
    ResolvedExpr left = resolve_expr(expr->binary_expr.left, NULL, is_global);
    ResolvedExpr right = resolve_expr(expr->binary_expr.right, NULL, is_global);
    if (left.type != right.type) {
        resolve_error(expr->loc, "type mismatch in the binary expression");
    }
    if (left.type != type_int) {
        resolve_error(expr->loc, "expression is not arithmetic type");
    }
    if (left.is_folded && right.is_folded) {        
        return (ResolvedExpr) {
            .value = eval_const_binary_expr(expr->binary_expr.op, left.value, right.value, expr->loc),
            .type = type_int,
            .is_lvalue = false,
            .is_const = left.is_const && right.is_const,
            .is_folded = true
        };
    }
    else {
        return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false, .is_folded = false };
    }
}

ResolvedExpr resolve_pre_unary_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_PRE_UNARY);
    ResolvedExpr base_expr = resolve_expr(expr->pre_unary_expr.expr, NULL, is_global);
    switch (expr->pre_unary_expr.op) {
    case TOKEN_INC:
        if (base_expr.is_const) {
            resolve_error(expr->loc, "const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            resolve_error(expr->loc, "an lvalue is expected as the operand");
        }
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        // unary increments are not folded and are not allowed in var expressions
        return (ResolvedExpr) { .type = base_expr.type, .is_lvalue = true, .is_const = false, .is_folded = false };
    case TOKEN_DEC:
        if (base_expr.is_const) {
            resolve_error(expr->loc, "const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            resolve_error(expr->loc, "an lvalue is expected as the operand");
        }
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        // unary decrements are not folded and are not allowed in var expressions
        return (ResolvedExpr) { .type = base_expr.type, .is_lvalue = true, .is_const = false, .is_folded = false };
    case '+':
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        return (ResolvedExpr) { .value = +base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const, .is_folded = base_expr.is_folded };
    case '-':
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        return (ResolvedExpr) { .value = -base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const, .is_folded = base_expr.is_folded };
    case '*':
        if (base_expr.type->type != TYPE_PTR) {
            resolve_error(expr->loc, "pointer type is expected in a dereference");
        }
        return (ResolvedExpr) { .type = base_expr.type->ptr.base, .is_lvalue = true, .is_const = false, .is_folded = false };
    case '&':
        if (!base_expr.is_lvalue) {
            resolve_error(expr->loc, "an lvalue is expected as the operand");
        }
        return (ResolvedExpr) { .type = type_ptr(base_expr.type), .is_lvalue = false, .is_const = false, .is_folded = false };
    case '!':
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        return (ResolvedExpr) { .value = !base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const, .is_folded = base_expr.is_folded };
    case '~':
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        return (ResolvedExpr) { .value = ~base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const, .is_folded = base_expr.is_folded };
    default:
        assert(0);
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_post_unary_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_POST_UNARY);
    ResolvedExpr base_expr = resolve_expr(expr->post_unary_expr.expr, NULL, is_global);
    switch (expr->post_unary_expr.op) {
    case TOKEN_INC:
        if (base_expr.is_const) {
            resolve_error(expr->loc, "const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            resolve_error(expr->loc, "an lvalue is expected as the operand");
        }
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        // unary increments are not folded and are not allowed in var expressions
        return (ResolvedExpr) { .type = base_expr.type, .is_lvalue = false, .is_const = false, .is_folded = false };
    case TOKEN_DEC:
        if (base_expr.is_const) {
            resolve_error(expr->loc, "const values cannot be incremented");
        }
        if (!base_expr.is_lvalue) {
            resolve_error(expr->loc, "an lvalue is expected as the operand");
        }
        if (base_expr.type != type_int) {
            resolve_error(expr->loc, "expression is not arithmetic type");
        }
        // unary decrements are not folded and are not allowed in var expressions
        return (ResolvedExpr) { .type = base_expr.type, .is_lvalue = false, .is_const = false, .is_folded = false };
    default:
        assert(0);
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_int_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_INT);
    return (ResolvedExpr) { .value = (int64_t)expr->int_expr.int_val, .type = type_int, .is_lvalue = false, .is_const = true, .is_folded = true };
}

ResolvedExpr resolve_float_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_FLOAT);
    return (ResolvedExpr) { .value = *(int64_t*)&(expr->float_expr.float_val), .type = type_float, .is_lvalue = false, .is_const = true, .is_folded = true };
}

ResolvedExpr resolve_name_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_NAME);
    return resolve_name(expr->name_expr.name, is_global);
}

ResolvedExpr resolve_index_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_INDEX);
    ResolvedExpr base_expr = resolve_expr(expr->index_expr.expr, NULL, is_global);
    ResolvedExpr index_expr = resolve_expr(expr->index_expr.index, NULL, is_global);
    if (base_expr.type->type != TYPE_PTR && base_expr.type->type != TYPE_ARRAY) {
        resolve_error(expr->loc, "An array or ptr is expected as the operand of an array index");
    }
    if (index_expr.type != type_int) {
        resolve_error(expr->loc, "An integer is expected as array index");
    }
    Type* type = base_expr.type->type == TYPE_ARRAY ? base_expr.type->array.base : base_expr.type->ptr.base;
    return (ResolvedExpr) { .type = type, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const, .is_folded = false };
}

ResolvedExpr resolve_field_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_FIELD);
    ResolvedExpr base_expr = resolve_expr(expr->field_expr.expr, NULL, is_global);
    for (size_t i = 0; i < base_expr.type->aggregate.num_fields; i++) {
        if (expr->field_expr.field == base_expr.type->aggregate.fields[i].name) {
            Type* type = base_expr.type->aggregate.fields[i].type;
            complete_type(type);
            return (ResolvedExpr) { .type = type, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const, .is_folded = false };
        }
    }
    resolve_error(expr->loc, "%s is not a field of %s", expr->field_expr.field, base_expr.type->entity->name);
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_sizeof_expr_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_SIZEOF_EXPR);
    ResolvedExpr base_expr = resolve_expr(expr->sizeof_expr.expr, NULL, is_global);
    return (ResolvedExpr) { .value = base_expr.type->size, .type = type_int, .is_lvalue = false, .is_const = true, .is_folded = true };
}

ResolvedExpr resolve_sizeof_type_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_SIZEOF_TYPE);
    Type* type = resolve_typespec(expr->sizeof_expr.type);
    complete_type(type);
    return (ResolvedExpr) { .value = type->size, .type = type_int, .is_lvalue = false, .is_const = true, .is_folded = true };
}

ResolvedExpr resolve_ternary_expr(Expr* expr, Type* expected_type, bool is_global) {
    assert(expr->type == EXPR_TERNARY);
    ResolvedExpr cond_expr = resolve_expr(expr->ternary_expr.cond, NULL, is_global);
    ResolvedExpr left_expr = resolve_expr(expr->ternary_expr.left, expected_type, is_global);
    ResolvedExpr right_expr = resolve_expr(expr->ternary_expr.right, expected_type, is_global);
    if (cond_expr.type != type_int) {
        resolve_error(expr->loc, "An integer type is expected in a condition expr in the ternary expr");
    }
    if (expected_type && left_expr.type != expected_type) {
        resolve_error(expr->loc, "The left expr type mismatch with the expected type");
    }
    if (expected_type && right_expr.type != expected_type) {
        resolve_error(expr->loc, "The right expr type mismatch with the expected type");
    }
    if (cond_expr.is_folded) {
        return cond_expr.value ? left_expr : right_expr;
    }
    else {
        return (ResolvedExpr) { .type = left_expr.type, .is_lvalue = left_expr.is_lvalue && right_expr.is_lvalue, .is_const = false, .is_folded = false };
    }
}

ResolvedExpr resolve_str_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_STR);
    return (ResolvedExpr) { .type = type_ptr(type_char), .is_lvalue = false, .is_const = true, .is_folded = false };
}

ResolvedExpr resolve_call_expr(Expr* expr, Type* expected_type, bool is_global) {
    assert(expr->type == EXPR_CALL);
    ResolvedExpr func_expr = resolve_expr(expr->call_expr.expr, NULL, is_global);
    if (expr->call_expr.num_args != func_expr.type->func.num_params) {
        resolve_error(expr->loc, "Argument count in the call expr mismatch with the function type");
    }
    for (size_t i = 0; i < expr->call_expr.num_args; i++) {
        ResolvedExpr param_expr = resolve_expr(expr->call_expr.args[i], func_expr.type->func.params[i], is_global);
        if (param_expr.type != func_expr.type->func.params[i]) {
            resolve_error(expr->loc, "Argument in the call expression mismatch with the function argument type");
        }
    }
    if (expected_type && func_expr.type->func.ret != expected_type) {
        resolve_error(expr->loc, "Function return type mismatch with the expected type");
    }
    return (ResolvedExpr) { .type = func_expr.type->func.ret, .is_lvalue = false, .is_const = false, .is_folded = false };
}

ResolvedExpr resolve_compound_expr(Expr* expr, Type* expected_type, bool is_global) {
    assert(expr->type == EXPR_COMPOUND);
    TypeSpec* compound_typespec = expr->compound_expr.type;
    if (!expected_type && !compound_typespec) {
        resolve_error(expr->loc, "Implicit types for compound expresssions are not allowed");
    }
    Type* compound_type = expected_type;
    if (expected_type && compound_typespec) {
        if (resolve_typespec(compound_typespec) != expected_type) {
            resolve_error(expr->loc, "Explicit type of the compound expression mismatch with the expected type");
        }
    }
    else if (compound_typespec) {
        compound_type = resolve_typespec(compound_typespec);
    }
    complete_type(compound_type);
    if (compound_type->type == TYPE_STRUCT || compound_type->type == TYPE_UNION) {
        if (compound_type->aggregate.num_fields < expr->compound_expr.num_compound_items) {
            resolve_error(expr->loc, "Number of fields in the compound expression exceeds the aggregate definition field count");
        }
        bool is_const = true;
        bool is_folded = true;
        for (size_t i = 0, k = 0; i < expr->compound_expr.num_compound_items; i++, k++) {
            CompoundItem compound_item = expr->compound_expr.compound_items[i];
            if (compound_item.type == COMPOUND_INDEX) {
                resolve_error(expr->loc, "Index items are not allowed in aggregate compound expressions");
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
                resolve_error(expr->loc, "Aggregate compound expresssion exceeds expr count");
            }
            ResolvedExpr r_expr = resolve_expr(compound_item.value, compound_type->aggregate.fields[k].type, is_global);
            is_const &= r_expr.is_const;
            is_folded &= r_expr.is_folded;
            if (compound_type->aggregate.fields[i].type != r_expr.type) {
                resolve_error(expr->loc, "Field type mismatch with the aggregate definition");
            }
        }
        return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = is_const, .is_folded = is_folded };
    }
    else if (compound_type->type == TYPE_ARRAY) {
        if (compound_type->array.size < expr->compound_expr.num_compound_items) {
            resolve_error(expr->loc, "Number of fields in the compound expression exceeds the array size");
        }
        bool is_const = true;
        bool is_folded = true;
        for (size_t i = 0, k = 0; i < expr->compound_expr.num_compound_items; i++, k++) {
            CompoundItem compound_item = expr->compound_expr.compound_items[i];
            if (compound_item.type == COMPOUND_NAME) {
                resolve_error(expr->loc, "Named items are not allowed in array compound expressions");
            }
            if (compound_item.type == COMPOUND_INDEX) {
                ResolvedExpr index_expr = resolve_expr(compound_item.index, NULL, is_global);
                if (index_expr.type != type_int && index_expr.type != type_char) {
                    resolve_error(expr->loc, "An int or char expr is expected in indices of array compound expressions");
                }
                if (!index_expr.is_const) {
                    resolve_error(expr->loc, "Const indices are expected in array compound expressions");
                }
                if (index_expr.value < 0) {
                    resolve_error(expr->loc, "Index expr should be non-negative");
                }
                k = (size_t) index_expr.value;
            }
            if (k >= compound_type->array.size) {
                resolve_error(expr->loc, "Index exceeds the array length in the array compound expression");
            }
            ResolvedExpr r_expr = resolve_expr(expr->compound_expr.compound_items[i].value, compound_type->array.base, is_global);
            is_const &= r_expr.is_const;
            is_folded &= r_expr.is_folded;
            if (compound_type->array.base != r_expr.type) {
                resolve_error(expr->loc, "Field type mismatch with the array type");
            }
        }
        return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = is_const, .is_folded = is_folded };
    }
    else if(compound_type) {
        if (expr->compound_expr.num_compound_items != 1) {
            resolve_error(expr->loc, "Only one element is expected in non array or non aggregate type compound expressions");
        }
        ResolvedExpr v_expr = resolve_expr(expr->compound_expr.compound_items[0].value, NULL, is_global);
        if (v_expr.type != type_int) {
            resolve_error(expr->loc, "Only integer type is expected in non array or non aggregate type compound expressions");
        }
        return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = v_expr.is_const, .is_folded = v_expr.is_folded };
        //resolve_error(expr->loc, "An array or struct or union type is expected in a compound expression");
    }
    return (ResolvedExpr) { 0 };
}

ResolvedExpr resolve_cast_expr(Expr* expr, bool is_global) {
    assert(expr->type == EXPR_CAST);
    Type* cast_type = resolve_typespec(expr->cast_expr.cast_type);
    ResolvedExpr cast_expr = resolve_expr(expr->cast_expr.cast_expr, NULL, is_global);
    if (cast_type->type == TYPE_PTR) {
        if (cast_expr.type->type != TYPE_PTR && cast_expr.type->type != TYPE_INT) {
            resolve_error(expr->loc, "Unsupported cast to a pointer");
        }
    }
    else if (cast_type->type == TYPE_INT) {
        if (cast_expr.type->type == TYPE_PTR 
            && cast_expr.type->type != TYPE_INT 
            && cast_expr.type->type != TYPE_FLOAT 
            && cast_expr.type->type == TYPE_CHAR) {
            resolve_error(expr->loc, "Unsupported cast to an int");
        }
    }
    else {
        resolve_error(expr->loc, "Unsupported cast");
    }
    return (ResolvedExpr) { .type = cast_type, .is_lvalue = false, .is_const = false, .is_folded = true };
}

Type* resolve_typespec_name(TypeSpec* typespec);
Type* resolve_typespec_func(TypeSpec* typespec);
Type* resolve_typespec_array(TypeSpec* typespec);
Type* resolve_typespec_ptr(TypeSpec* typespec);

Type* resolve_typespec(TypeSpec* typespec) {
    Type* type = NULL;
    switch (typespec->type) {
    case TYPESPEC_NAME:
        type = resolve_typespec_name(typespec);
        break;
    case TYPESPEC_FUNC:
        type = resolve_typespec_func(typespec);
        break;
    case TYPESPEC_ARRAY:
        type = resolve_typespec_array(typespec);
        break;
    case TYPESPEC_PTR:
        type = resolve_typespec_ptr(typespec);
        break;
    default:
        assert(0);
    }
    if (type) {
        complete_type(type);
    }
    return type;
}

Type* resolve_typespec_name(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_NAME);
    const char* name = typespec->name.name;
    Entity* entity = get_entity(name);
    if (!entity) {
        resolve_error(typespec->loc, "Typespec name %s is not found", name);
    }
    resolve_entity(entity);
    if (entity->e_type != ENTITY_TYPE) {
        resolve_error(typespec->loc, "A type name is expected. Got %s", name);
    }
    typespec->resolved_type = entity->type;
    return entity->type;
}

Type* resolve_typespec_func(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_FUNC);
    Type** params = xcalloc(typespec->func.num_params, sizeof(Type*));
    for (size_t i = 0; i < typespec->func.num_params; i++) {
        params[i] = resolve_typespec(typespec->func.params[i]);
    }
    Type* ret_type = resolve_typespec(typespec->func.ret_type);
    Type* result = type_func(typespec->func.num_params, params, ret_type);
    typespec->resolved_type = result;
    return result;
}

Type* resolve_typespec_array(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_ARRAY);
    ResolvedExpr size_expr = resolve_expr(typespec->array.size, NULL, false);
    if (!size_expr.is_const) {
        resolve_error(typespec->loc, "const is expected in an array size expression");
    }
    if (size_expr.value < 0 || (uint64_t)size_expr.value > SIZE_MAX) {
        resolve_error(typespec->loc, "invalid expr value for array size");
    }
    Type* result = type_array(resolve_typespec(typespec->array.base), (size_t)size_expr.value);
    typespec->resolved_type = result;
    return result;
}

Type* resolve_typespec_ptr(TypeSpec* typespec) {
    assert(typespec->type == TYPESPEC_PTR);
    Type* result = type_ptr(resolve_typespec(typespec->ptr.base));
    typespec->resolved_type = result;
    return result;
}

void complete_type(Type* type) {
    if (type->type == TYPE_COMPLETING) {
        resolve_error(type->entity->loc, "Cyclic dependancy in %s", type->entity->name);
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
                ResolvedExpr aggregate_expr = resolve_expr(aggregate_decl.aggregate_items[i].expr, NULL, false);
                if (aggregate_type != aggregate_expr.type) {
                    resolve_error(type->entity->loc, "aggregate type and expression inferred type mismatch");
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
        resolve_error(entity->loc, "Cyclic dependancy for %s", entity->name);
    }
    else if (entity->state == ENTITY_STATE_UNRESOVLED) {
        entity->state = ENTITY_STATE_RESOLVING;
        switch (entity->e_type) {
        case ENTITY_TYPE: // typedef only
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
        ResolvedExpr r_expr = resolve_expr(entity->decl->var_decl.expr, type, true);
        if (type && type != r_expr.type) {
            resolve_error(entity->loc, "declared type and the expression types mismatch in %s", entity->name);
        }
        if (!r_expr.is_folded) {
            resolve_error(entity->loc, "declared expr of %s is not evaluable at compile time", entity->name);
        }
        entity->value = r_expr.value;
        entity->is_set = true;
        entity->is_folded = r_expr.is_folded;//r_expr.type == type_int;
        type = r_expr.type;
        entity->type = type;
    }
    complete_type(type);
    buf_push(ordered_entities, entity);
}

#if 0

ok
==


not ok
=======
any expr involving compiler time in-evaluable

#endif

void resolve_entity_const(Entity* entity) {
    ResolvedExpr r_expr = resolve_expr(entity->decl->const_decl.expr, NULL, true);
    if (!r_expr.is_const) {
        resolve_error(entity->loc, "A const expr is expected in the const declaration %s", entity->name);
    }
    entity->type = r_expr.type;
    entity->value = r_expr.value;
    entity->is_set = true;
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
    entity->is_set = true;
    buf_push(ordered_entities, entity);
}

Entity* entity_local_var(const char* name) {
    Entity* entity = entity_alloc(ENTITY_LOCAL);
    entity->name = name;
    entity->state = ENTITY_STATE_RESOLVED;
    return entity;
}

void resolve_cond_expr(Expr* cond_expr, BlockStmnt block, Type* ret_type) {
    ResolvedExpr cond = resolve_expr(cond_expr, type_int, false);
    if (cond.type != type_int) {
        resolve_error(cond_expr->loc, "int type is expected in an if condition expr");
    }
    resolve_stmnt_block(block, ret_type);
}

void resolve_stmnt(Stmnt* stmnt, Type* ret_type) {
    if (!stmnt) {
        return;
    }
    switch (stmnt->type) {
    case STMNT_DECL: {
        assert(stmnt->decl_stmnt.decl->type == DECL_VAR);
        Decl* decl = stmnt->decl_stmnt.decl;
        Entity* entity = entity_local_var(decl->name);
        if (decl->var_decl.type) {
            entity->type = resolve_typespec(decl->var_decl.type);
        }
        else if (decl->var_decl.expr) {
            entity->type = resolve_expr(decl->var_decl.expr, NULL, false).type;
        }
        else {
            resolve_error(stmnt->loc, "Either type or expr is expected in a var declaration");
        }
        push_local_entity(entity);
        break;
    }
    case STMNT_RETURN: { 
        ResolvedExpr return_expr = resolve_expr(stmnt->return_stmnt.expr, ret_type, false);
        if (return_expr.type != ret_type) {
            resolve_error(stmnt->loc, "Return statement type mismatch with the function return type");
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
        ResolvedExpr switch_expr = resolve_expr(stmnt->switch_stmnt.switch_expr, NULL, false);
        for (size_t i = 0; i < stmnt->switch_stmnt.num_case_blocks; i++) {
            ResolvedExpr cond = resolve_expr(stmnt->switch_stmnt.case_blocks[i].case_expr, switch_expr.type, false);
            if (cond.type != switch_expr.type) {
                resolve_error(stmnt->loc, "switch type is expected in a case condition expr");
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
        ResolvedExpr cond = resolve_expr(stmnt->for_stmnt.cond, type_int, false);
        if (cond.type != type_int) {
            resolve_error(stmnt->loc, "An int is expected in the condition of a for stmnt");
        }
        for (size_t i = 0; i < stmnt->for_stmnt.num_update; i++) {
            resolve_stmnt(stmnt->for_stmnt.update[i], NULL);
        }
        resolve_stmnt_block(stmnt->for_stmnt.block, ret_type);
        leave_scope(local_entity);
        break;
    }
    case STMNT_ASSIGN: {
        Entity* left_entity = get_expr_entity(stmnt->assign_stmnt.left);
        if (left_entity) {
            left_entity->is_set = true;
        }
        bool left_used = left_entity->is_used;
        ResolvedExpr left = resolve_expr(stmnt->assign_stmnt.left, NULL, false);
        left_entity->is_used = left_used;
        ResolvedExpr right = resolve_expr(stmnt->assign_stmnt.right, left.type, false);
        if (stmnt->assign_stmnt.op != '=') {
            if (left.type != type_int) {
                resolve_error(stmnt->loc, "An int is expected in the left operand of an assignment");
            }
            if (right.type != type_int) {
                resolve_error(stmnt->loc, "An int is expected in the right operand of an assignment");
            }
        }
        else {
            if (left.type != right.type) {
                resolve_error(stmnt->loc, "Left operand and right operand types should be matched in an assignment");
            }
        }
        if (!left.is_lvalue || left.is_const) {
            resolve_error(stmnt->loc, "A modifiable lvalue is expected as the left operand of an assignement");
        }
        break;
    }
    case STMNT_INIT: {
        if (stmnt->init_stmnt.left->type != EXPR_NAME) {
            resolve_error(stmnt->loc, "A name expression is expected as the left operand of an init statement");
        }
        ResolvedExpr right = resolve_expr(stmnt->init_stmnt.right, NULL, false);
        Entity* entity = entity_local_var(stmnt->init_stmnt.left->name_expr.name);
        entity->is_set = true;
        entity->type = right.type;
        push_local_entity(entity);
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
        resolve_expr(stmnt->expr_stmnt.expr, NULL, false);
        break;
    }
    default:
        assert(0);
    }
}

void resolve_stmnt_block(BlockStmnt block, Type* ret_type) {
    Entity** local_entity = enter_scope();
    for (size_t i = 0; i < block.num_stmnts; i++) {
        resolve_stmnt(block.stmnts[i], ret_type);
    }
    leave_scope(local_entity);
}

void resolve_func(Entity* entity) {
    assert(entity->e_type == ENTITY_FUNC);
    Decl* decl = entity->decl;
    Entity** local_entity = enter_scope();
    for (size_t i = 0; i < decl->func_decl.num_params; i++) {
        Entity* param = entity_local_var(decl->func_decl.params[i].name);
        param->type = resolve_typespec(decl->func_decl.params[i].type);
        push_local_entity(param);
    }
    resolve_stmnt_block(decl->func_decl.block, resolve_typespec(decl->func_decl.ret_type));
    leave_scope(local_entity);
}

void check_entity_usage(void) {
    for (KeyValPair* it = global_entities.pairs; it != global_entities.pairs + global_entities.cap; it++) {
        Entity* entity = it->val;
        if (entity) {
            if (entity->is_set && !entity->is_used) {
                resolve_warning(entity->loc, "warning: %s is set but never used", entity->name);
            }
            else if (!entity->is_set && entity->is_used) {
                resolve_warning(entity->loc, "warning: %s is used without setting", entity->name);
            }
            else if (!entity->is_set && !entity->is_used) {
                resolve_warning(entity->loc, "warning: %s is not set nor used", entity->name);
            }
        }
    }
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
    for (KeyValPair* it = global_entities.pairs; it != global_entities.pairs + global_entities.cap; it++) {
        if (it->val) {
            complete_entity(it->val);
        }
    }
    check_entity_usage();
}

#pragma TODO("Align fields in aggregate types")
#pragma TODO("Do pointer decaying for arrays")

void resolve_decl_test(void) {
    install_built_in_types();
    install_built_in_consts();
    //const char* src[] = {
    //    "struct V {x: int; y: int;}",
    //    "var v: V = {y=1, x=2}",
    //    "var w: int[3] = {1, [2]=3}",
    //    "var vv: V[2] = {{1, 2}, {3, 4}}",
    //    "func add(a: V, b: V): V { var c: V; c = {a.x + b.x, a.y + b.y}; return c; }",
    //    "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}",
    //    "func printf(a: int) {return;}",
    //    "func one(n: int) { if(n < 0) {return;} var i:int; for(i = 0; i < n; i++) {printf(1);}}",
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
    //};
    const char* src = "struct V {x: int; y: int;}\n"
        "var v: V = {y=1, x=2}\n"
        "var w: int[3] = {1, [2]=3}\n"
        "var vv: V[2] = {{1, 2}, {3, 4}}\n"
        "func add(a: V, b: V): V { var c: V; c = {a.x + b.x, a.y + b.y}; return c; }\n"
        "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}\n"
        "func f(): void {j := 0; for(i := 0, j = 1; i < 10; i++, j++) { fib(i + j); }}";

    init_stream(src);
    DeclSet* declset = parse_stream();
    install_decls(declset);
    complete_entities();
    for (Entity** it = ordered_entities; it != buf_end(ordered_entities); it++) {
        print_decl((*it)->decl);
        printf("\n");
    }
}

void resolve_test(void) {
    printf("----- resolve.c -----\n");
    resolve_decl_test();
    printf("resolve test passed");
}
