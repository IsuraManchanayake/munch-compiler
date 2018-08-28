typedef struct Type Type;
typedef struct Entity Entity;

typedef enum TypeType {
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
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

struct Type {
    TypeType type;
    size_t size;
    Entity* entity;
    union {
        struct {
            Type* base;
        } ptr;
        struct {
            size_t num_params;
            Type** params;
            Type* ret;
        } func;
        struct {
            size_t num_fields;
            TypeField* fields;
        } aggregate;
        struct {
            Type* base;
            size_t size;
        } array;
    };
};

Type* type_alloc(TypeType type_type) {
    Type* type = xcalloc(1, sizeof(Type));
    type->type = type_type;
    return type;
}

#define INT_SIZE 4
#define FLOAT_SIZE 4
#define PTR_SIZE 4

Type _type_int_val = { .type = TYPE_INT, .size = INT_SIZE };
Type _type_float_val = { .type = TYPE_FLOAT, .size = FLOAT_SIZE };

Type* type_float = &_type_float_val;
Type* type_int = &_type_int_val;

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
    buf_push(cached_ptr_types, ((CachedPtrType) { .base = base, .ptr = type}));
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
    buf_push(cached_array_types, ((CachedArrayType){ .base = base, .size = size, .array = type }));
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

Entity** entities = NULL;
Entity** ordered_entities = NULL;

Entity* get_entity(const char* name) {
    for (size_t i = 0; i < buf_len(entities); i++) {
        if (entities[i]->name == name) {
            return entities[i];
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
            buf_push(entities, enum_entity);
        }
    }
    buf_push(entities, entity);
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

#define _BUILT_IN_TYPE(t) buf_push(entities, built_in_type(type_ ## t, #t));

void install_built_in_types() {
    _BUILT_IN_TYPE(int);
    _BUILT_IN_TYPE(float);
}

#undef _BUILT_IN_TYPE

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
    else {
        fatal("A value expression is expected by %s", name);
    }
}

ResolvedExpr resolve_binary_expr(Expr* expr);
ResolvedExpr resolve_pre_unary(Expr* expr);
ResolvedExpr resolve_post_unary(Expr* expr);

void complete_type(Type* type);
Type* resolve_typespec(TypeSpec* typespec);

ResolvedExpr resolve_expr(Expr* expr, Type* expected_type) {
    switch (expr->type) {
    case EXPR_BINARY:
        return resolve_binary_expr(expr);
    case EXPR_PRE_UNARY:
        return resolve_pre_unary(expr);
    case EXPR_POST_UNARY:
        return resolve_post_unary(expr);
    case EXPR_INT:
        return (ResolvedExpr){ .value = (int64_t) expr->int_expr.int_val, .type = type_int, .is_lvalue = false, .is_const = true };
    case EXPR_FLOAT:
        return (ResolvedExpr) { .value = expr->float_expr.float_val, .type = type_float, .is_lvalue = false, .is_const = true };
    case EXPR_NAME:
        return resolve_name(expr->name_expr.name);
    case EXPR_INDEX: {
        ResolvedExpr base_expr = resolve_expr(expr->index_expr.expr, NULL);
        return (ResolvedExpr) { .type = base_expr.type->array.base, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const };
    }
    case EXPR_FIELD: {
        ResolvedExpr base_expr = resolve_expr(expr->field_expr.expr, NULL);
        for (size_t i = 0; i < base_expr.type->aggregate.num_fields; i++) {
            if (expr->field_expr.field == base_expr.type->aggregate.fields[i].name) {
                Type* type = base_expr.type->aggregate.fields[i].type;
                complete_type(type);
                return (ResolvedExpr) { .type = type, .is_lvalue = base_expr.is_lvalue, .is_const = base_expr.is_const };
            }
        }
        fatal("%s is not a field of %s", expr->field_expr.field, base_expr.type->entity->name);
    }
    case EXPR_SIZEOF_TYPE: {
        Type* type = resolve_typespec(expr->sizeof_expr.type);
        complete_type(type);
        return (ResolvedExpr) { .value = type->size, .type = type_int, .is_lvalue = false, .is_const = true };
    }
    case EXPR_SIZEOF_EXPR: {
        ResolvedExpr base_expr = resolve_expr(expr->sizeof_expr.expr, NULL);
        return (ResolvedExpr) { .value = base_expr.type->size, .type = type_int, .is_lvalue = false, .is_const = true };
    }
    case EXPR_TERNARY:
    case EXPR_CALL:
    case EXPR_STR:
        assert(0);
    case EXPR_COMPOUND: {
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
        else if(compound_typespec) {
            compound_type = resolve_typespec(compound_typespec);
        }
        complete_type(compound_type);
        if (compound_type->type == TYPE_STRUCT || compound_type->type == TYPE_UNION) {
            if (compound_type->aggregate.num_fields < expr->compound_expr.num_exprs) {
                fatal("Number of fields in the compound expression exceeds the aggregate definition field count");
            }
            bool is_const = true;
            for (size_t i = 0; i < expr->compound_expr.num_exprs; i++) {
                ResolvedExpr r_expr = resolve_expr(expr->compound_expr.exprs[i], NULL);
                is_const &= r_expr.is_const;
                if (compound_type->aggregate.fields[i].type != r_expr.type) {
                    fatal("Field type mismatch with the aggregate definition");
                }
            }
            return (ResolvedExpr) { .type = compound_type, .is_lvalue = false, .is_const = is_const };
        }
        else if (compound_type->type == TYPE_ARRAY) {
            if (compound_type->array.size < expr->compound_expr.num_exprs) {
                fatal("Number of fields in the compound expression exceeds the array size");
            }
            bool is_const = true;
            for (size_t i = 0; i < expr->compound_expr.num_exprs; i++) {
                ResolvedExpr r_expr = resolve_expr(expr->compound_expr.exprs[i], NULL);
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
        break;
    }
    case EXPR_CAST:
    default:
        assert(0);
    }
}

#pragma TODO("Complete binary expr resolving")

ResolvedExpr resolve_binary_expr(Expr* expr) {
    assert(expr->type == EXPR_BINARY);
    ResolvedExpr left = resolve_expr(expr->binary_expr.left, NULL);
    ResolvedExpr right = resolve_expr(expr->binary_expr.right, NULL);
    switch (expr->binary_expr.op)
    {
    case '+':
        if (left.type != right.type) {
            fatal("type mismatch in the binary expression");
        }
        assert(left.type == type_int);
        if (left.is_const && right.is_const) {
            return (ResolvedExpr) { .value = left.value + right.value, .type = left.type, .is_lvalue = false, .is_const = true };
        }
        else {
            return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false };
        }
    case '-':
        if (left.type != right.type) {
            fatal("type mismatch in the binary expression");
        }
        assert(left.type == type_int);
        if (left.is_const && right.is_const) {
            return (ResolvedExpr) { .value = left.value - right.value, .type = left.type, .is_lvalue = false, .is_const = true };
        }
        else {
            return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false };
        }
    case '*':
        if (left.type != right.type) {
            fatal("type mismatch in the binary expression");
        }
        assert(left.type == type_int);
        if (left.is_const && right.is_const) {
            return (ResolvedExpr) { .value = left.value * right.value, .type = left.type, .is_lvalue = false, .is_const = true };
        }
        else {
            return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false };
        }
    case '/':
        if (left.type != right.type) {
            fatal("type mismatch in the binary expression");
        }
        assert(left.type == type_int);
        if (left.is_const && right.is_const) {
            return (ResolvedExpr) { .value = left.value / right.value, .type = left.type, .is_lvalue = false, .is_const = true };
        }
        else {
            return (ResolvedExpr) { .type = left.type, .is_lvalue = false, .is_const = false };
        }
    default:
        assert(0);
    }
}

ResolvedExpr resolve_pre_unary(Expr* expr) {
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
        return (ResolvedExpr) { .value = base_expr.value, .type = base_expr.type, .is_lvalue = false, .is_const = base_expr.is_const };
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
    case '~': 
    default:
        assert(0);
    }
}

ResolvedExpr resolve_post_unary(Expr* expr) {
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
}

Type* resolve_typespec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME: {
        const char* name = typespec->name.name;
        Entity* entity = get_entity(name);
        resolve_entity(entity);
        if (entity->e_type != ENTITY_TYPE) {
            fatal("A type name is expected. Got %s", name);
        }
        return entity->type;
    }
    case TYPESPEC_FUNC: {
        Type** params = xcalloc(typespec->func.num_params, sizeof(Type*));
        for (size_t i = 0; i < typespec->func.num_params; i++) {
            params[i] = resolve_typespec(typespec->func.params[i]);
        }
        Type* ret_type = resolve_typespec(typespec->func.ret_type);
        return type_func(typespec->func.num_params, params, ret_type);
    }
    case TYPESPEC_ARRAY: {
        ResolvedExpr size_expr = resolve_expr(typespec->array.size, NULL);
        if (!size_expr.is_const) {
            fatal("const is expected in an array size expression");
        }
        if (!is_between(size_expr.value, 0, SIZE_MAX)) {
            fatal("invalid expr value for array size");
        }
        return type_array(resolve_typespec(typespec->array.base), size_expr.value);
    }
    case TYPESPEC_PTR:
        return type_ptr(resolve_typespec(typespec->ptr.base));
    default:
        assert(0);
    }
}

void complete_type(Type* type) {
    if (type->type == TYPE_COMPLETING) {
        fatal("Cyclic dependancy in %s", type->entity->name);
    }
    else if (type->type == TYPE_INCOMPLETE) {
        type->type = TYPE_COMPLETING;
        TypeField* aggregate_fields = NULL;
        for (size_t i = 0; i < type->entity->decl->aggregate_decl.num_aggregate_items; i++) {
            Type* aggregate_type = resolve_typespec(type->entity->decl->aggregate_decl.aggregate_items[i].type);
            complete_type(aggregate_type);
            if (type->entity->decl->aggregate_decl.aggregate_items[i].expr) {
                ResolvedExpr aggregate_expr = resolve_expr(type->entity->decl->aggregate_decl.aggregate_items[i].expr, NULL);
                if (aggregate_type != aggregate_expr.type) {
                    fatal("aggregate type and expression inferred type mismatch");
                }
            }
            buf_push(aggregate_fields, ((TypeField) { .type = aggregate_type, .name = type->entity->decl->aggregate_decl.aggregate_items[i].name }));
        }
        if (type->entity->decl->type == DECL_STRUCT) {
            type_struct(type, type->entity->decl->aggregate_decl.num_aggregate_items, aggregate_fields);
        }
        else if (type->entity->decl->type == DECL_UNION) {
            type_union(type, type->entity->decl->aggregate_decl.num_aggregate_items, aggregate_fields);
        }
        else {
            assert(0);
        }
        buf_push(ordered_entities, type->entity);
    }
}

void resolve_entity(Entity* entity) {
    if (entity->state == ENTITY_STATE_RESOLVING) {
        fatal("Cyclic dependancy for %s", entity->name);
    } else if (entity->state == ENTITY_STATE_UNRESOVLED) {
        entity->state = ENTITY_STATE_RESOLVING;
        Type* type = NULL;
        switch (entity->e_type) {
        case ENTITY_TYPE:
            type = resolve_typespec(entity->decl->typedef_decl.type);
            entity->type = type;
            complete_type(type);
            buf_push(ordered_entities, entity);
            break;
        case ENTITY_VAR:
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
            break;
        case ENTITY_ENUM_CONST:
        case ENTITY_CONST: {
            ResolvedExpr r_expr = resolve_expr(entity->decl->const_decl.expr, NULL);
            if (!r_expr.is_const) {
                fatal("A const expr is expected in a const declaration");
            }
            entity->type = r_expr.type;
            entity->value = r_expr.value;
            buf_push(ordered_entities, entity);
            break;
        }
        case ENTITY_FUNC:
            for (size_t i = 0; i < entity->decl->func_decl.num_params; i++) {
                type = resolve_typespec(entity->decl->func_decl.params[i].type);
                complete_type(type);
            }
            if (entity->decl->func_decl.ret_type) {
                type = resolve_typespec(entity->decl->func_decl.ret_type);
                complete_type(type);
            }
            buf_push(ordered_entities, entity);
            break;
        default:
            assert(0);
        }
        entity->state = ENTITY_STATE_RESOLVED;
    }
}

void complete_entity(Entity* entity) {
    resolve_entity(entity);
    if (entity->e_type == ENTITY_TYPE) {
        complete_type(entity->type);
    }
}

void complete_entities() {
    for (Entity** it = entities; it != buf_end(entities); it++) {
        complete_entity(*it);
    }
}

resolve_decl_test() {
    install_built_in_types();
    const char* src[] = {
        "struct PP {x:int; y:int;}",
        "var ppa = &a",
        "var v = (:PP){1, Q}",
        "enum A{P, Q=5+3*3, R, S}",
        "union AA {x:int; y:int*;}",
        "var z = AA {1, ppa}",
        "var wer = (:int[3]){1, 3, 4}",
        "const a = b",
        "const b = 3",
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

resolve_test() {
    printf("----- resolve.c -----\n");
    resolve_decl_test();
    printf("resolve test passed");
}