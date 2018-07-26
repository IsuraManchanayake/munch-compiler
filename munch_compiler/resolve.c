typedef enum TypeType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STR,
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

const Type _type_int_val = { TYPE_INT };
const Type _type_float_val = { TYPE_FLOAT };
const Type _type_str_val = { TYPE_STR };

const Type* type_float = &_type_int_val;
const Type* type_int = &_type_float_val;
const Type* type_str = &_type_str_val;

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
    buf_push(cached_func_types, ((CachedFuncType) {.num_params = num_params, .params = params, .ret = ret, .func = type }));
    return type;
}

Type* type_struct(size_t num_fields, TypeField* fields) {
    Type* type = type_alloc(TYPE_STRUCT);
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
    return type;
}

Type* type_union(size_t num_fields, TypeField* fields) {
    Type* type = type_alloc(TYPE_UNION);
    type->aggregate.num_fields = num_fields;
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
    return type;
}

typedef enum SymState {
    SYM_STATE_UNRESOLVED,
    SYM_STATE_RESOLVING,
    SYM_STATE_RESOLVED
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

void sym_push(Decl* decl) {
    assert(decl->name);
    if (sym_get(decl->name)) {
        fatal("Symbol %s is already defined", decl->name);
    }
    buf_push(syms, ((Sym) { decl->name, SYM_STATE_UNRESOLVED, decl }));
}

Sym* resolve_decl(Decl* decl) {
    Sym* sym = NULL;
    switch (decl->type) {
    case DECL_CONST:
        break;
    }
    return sym;
}

Sym* resolve_sym(Sym* sym) {
    switch (sym->state)
    {
    case SYM_STATE_RESOLVED:
        return sym;
    case SYM_STATE_UNRESOLVED:
        sym->state = SYM_STATE_RESOLVING;
        Sym* sym_decl = resolve_decl(sym->decl);
        sym->state = SYM_STATE_RESOLVED;
        return sym_decl;
    case SYM_STATE_RESOLVING:
        fatal("Cyclic symbol dependencies for %s", sym->name);
    default:
        assert(0);
    }
    return NULL;
}

Sym* resolve_name(const char* name) {
    Sym* sym = sym_get(name);
    if (sym) {
        return sym;
    }
    fatal("Unresolved symbol %s", name);
}

resolve_syms() {
    for (Sym* it = syms; it != buf_end(syms); it++) {
        resolve_sym(it);
    }
}

resolve_test() {
    printf("----- resolve.c -----\n");

    const char* Pi = str_intern("Pi");

    Decl* d1 = decl_const(Pi, expr_float(3.14));
    sym_push(d1);
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

    Type* struct_1 = type_struct(2, (TypeField[]) { {type_str, "name"}, { type_int, "age" } });
    Type* struct_2 = type_struct(2, (TypeField[]) { {type_str, "name"}, { type_int, "age" } });
    Type* union_1 = type_union(2, (TypeField[]) { {type_str, "name"}, {type_int, "age"} });
    assert(struct_1 != struct_2);
    assert(struct_1 != union_1);

    printf("resolve test passed");
}