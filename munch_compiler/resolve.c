typedef enum TypeType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PTR,
    TYPE_FUNC,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ARRAY
} TypeType;

typedef struct Type Type;

typedef struct TypeField {
    Type* type;
    const char* name;
} TypeField;

struct Type {
    TypeType type;
    size_t size;
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

Type _type_int_val = { TYPE_INT, INT_SIZE };
Type _type_float_val = { TYPE_FLOAT, FLOAT_SIZE };

Type* type_float = &_type_int_val;
Type* type_int = &_type_float_val;

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

Type* type_struct(size_t num_fields, TypeField* fields) {
    Type* type = type_alloc(TYPE_STRUCT);
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++) {
        type->size += it->type->size;
    }
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
    return type;
}

Type* type_union(size_t num_fields, TypeField* fields) {
    Type* type = type_alloc(TYPE_UNION);
    type->size = 0;
    for (TypeField* it = fields; it != fields + num_fields; it++) {
        type->size = max(type->size, it->type->size);
    }
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
    return type;
}

typedef enum SymState {
    SYM_STATE_UNORDERED,
    SYM_STATE_ORDERING,
    SYM_STATE_ORDERED
} SymState;

typedef struct Sym {
    const char* name;
    SymState state;
    Decl* decl;
} Sym;

Sym* syms;

Sym* sym_get(const char* name) {
    for (Sym* it = syms; it != buf_end(syms); it++) {
        if (it->name == name) {
            return it;
        }
    }
    return NULL;
}

void sym_decl(Decl* decl) {
    assert(decl->name);
    if (sym_get(decl->name)) {
        fatal("Symbol %s is already defined", decl->name);
    }
    buf_push(syms, ((Sym) { decl->name, SYM_STATE_UNORDERED, decl }));
}

void sym_builtin(const char* name) {
    buf_push(syms, ((Sym) {str_intern(name), SYM_STATE_ORDERED, NULL}));
}

Decl** ordered_decls = NULL;

void order_decl(Decl*);
void order_typespec(TypeSpec*);
void order_expr(Expr*);

void order_name(const char* name) {
    Sym* sym = sym_get(name);
    if (!sym) {
        fatal("Name \"%s\" could not be found", name);
        return;
    }
    switch (sym->state) {
    case SYM_STATE_ORDERED:
        break;
    case SYM_STATE_ORDERING:
        fatal("Cyclic dependancy of \"%s\"", name);
        break;
    case SYM_STATE_UNORDERED:
        sym->state = SYM_STATE_ORDERING;
        order_decl(sym->decl);
        sym->state = SYM_STATE_ORDERED;
        buf_push(ordered_decls, sym->decl);
        break;
    default:
        assert(0);
    }
}

void order_typespec(TypeSpec* type) {
    switch (type->type) {
    case TYPESPEC_NAME:
        order_name(type->name.name);
        break;
    case TYPESPEC_FUNC:
        if (type->func.ret_type) {
            order_typespec(type->func.ret_type);
        }
        for (TypeSpec** it = type->func.params; it != type->func.params + type->func.num_params; it++) {
            order_typespec(*it);
        }
        break;
    case TYPESPEC_ARRAY:
        order_typespec(type->array.base);
        order_expr(type->array.size);
        break;
    case TYPESPEC_PTR:
        order_typespec(type->ptr.base);
        break;
    default:
        assert(0);
    }
}

void order_expr(Expr* expr) {
    switch (expr->type)
    {
    case EXPR_TERNARY:
        order_expr(expr->ternary_expr.cond);
        order_expr(expr->ternary_expr.left);
        order_expr(expr->ternary_expr.right);
        break;
    case EXPR_BINARY:
        order_expr(expr->binary_expr.left);
        order_expr(expr->binary_expr.right);
        break;
    case EXPR_PRE_UNARY:
    case EXPR_POST_UNARY:
        order_expr(expr->unary_expr.expr);
        break;
    case EXPR_CALL:
        order_expr(expr->call_expr.expr);
        for (Expr** it = expr->call_expr.args; it != expr->call_expr.args + expr->call_expr.num_args; it++) {
            order_expr(*it);
        }
        break;
    case EXPR_INT:
    case EXPR_FLOAT:
    case EXPR_STR:
        break;
    case EXPR_NAME:
        order_name(expr->name_expr.name);
        break;
    case EXPR_COMPOUND:
        if (expr->compound_expr.type) {
            order_typespec(expr->compound_expr.type);
        }
        for (Expr** it = expr->compound_expr.exprs; it != expr->compound_expr.exprs + expr->compound_expr.num_exprs; it++) {
            order_expr(*it);
        }
        break;
    case EXPR_CAST:
        order_typespec(expr->cast_expr.cast_type);
        order_expr(expr->cast_expr.cast_expr);
        break;
    case EXPR_INDEX:
        order_expr(expr->index_expr.expr);
        order_expr(expr->index_expr.index);
        break;
    case EXPR_FIELD:
        order_expr(expr->field_expr.expr);
        break;
    case EXPR_SIZEOF_TYPE:
        order_typespec(expr->sizeof_expr.type);
        break;
    case EXPR_SIZEOF_EXPR:
        order_expr(expr->sizeof_expr.expr);
        break;
    default:
        assert(0);
    }
}

void order_decl(Decl* decl) {
    switch (decl->type) {
    case DECL_ENUM:
        break;
    case DECL_STRUCT:
    case DECL_UNION:
        for (AggregateItem* it = decl->aggregate_decl.aggregate_items; it != decl->aggregate_decl.aggregate_items + decl->aggregate_decl.num_aggregate_items; it++) {
            order_typespec(it->type);
            if (it->expr) {
                order_expr(it->expr);
            }
        }
        break;
    case DECL_CONST:
        order_expr(decl->const_decl.expr);
        break;
    case DECL_VAR:
        if (decl->var_decl.expr) {
            order_expr(decl->var_decl.expr);
        }
        if (decl->var_decl.type) {
            order_typespec(decl->var_decl.type);
        }
        break;
    case DECL_TYPEDEF:
        order_typespec(decl->typedef_decl.type);
        break;
    case DECL_FUNC:
        // do nothing
        break;
    default:
        assert(0);
    }
}

void order_decls() {
    for (Sym* it = syms; it != buf_end(syms); it++) {
        order_name(it->name);
    }
}

type_test() {
    const char* Pi = str_intern("Pi");

    Decl* d1 = decl_const(Pi, expr_float(3.14));
    sym_decl(d1);
    assert(sym_get(Pi)->decl == d1);

    Type* int_3_array = type_array(type_int, 3);
    assert(int_3_array->type == TYPE_ARRAY);
    Type* int_4_array_1 = type_array(type_int, 4);
    assert(int_4_array_1 != int_3_array);
    Type* int_4_array_2 = type_array(type_int, 4);
    assert(int_4_array_1 == int_4_array_2);

    Type* int_to_int_func_1 = type_func(1, &type_int, type_int);
    assert(int_to_int_func_1->type == TYPE_FUNC);
    Type* int_to_void_func = type_func(1, &type_int, NULL);
    assert(int_to_void_func != int_to_int_func_1);
    Type* void_to_int_func_1 = type_func(0, NULL, type_int);
    assert(void_to_int_func_1->func.num_params == 0);
    Type* void_to_int_func_2 = type_func(0, NULL, type_int);
    assert(void_to_int_func_1 == void_to_int_func_2);
    Type* int_to_int_func_2 = type_func(1, &type_int, type_int);
    assert(int_to_int_func_1 == int_to_int_func_2);

    Type* struct_1 = type_struct(2, (TypeField[]) { {type_float, "height"}, { type_int, "id" } });
    Type* struct_2 = type_struct(2, (TypeField[]) { {type_float, "height"}, { type_int, "id" } });
    Type* union_1 = type_union(2, (TypeField[]) { {type_float, "height"}, { type_int, "id" } });
    assert(struct_1 != struct_2);
    assert(struct_1 != union_1);
}

resolve_decl_test() {
    const char* src[] = {
        "const a = b",
        "const b = 3",
        "struct A { b : int = d; }",
        "var d: int = 1"
    };
    sym_builtin("int");
    for (int i = 0; i < sizeof(src) / sizeof(*src); i++) {
        init_stream(src[i]);
        Decl* decl = parse_decl();
        sym_decl(decl);
    }
    order_decls();
    for (Decl** it = ordered_decls; it != buf_end(ordered_decls); it++) {
        printf("%s\n", (*it)->name);
    }
}

resolve_test() {
    printf("----- resolve.c -----\n");
    //type_test();
    resolve_decl_test();
    printf("resolve test passed");
}