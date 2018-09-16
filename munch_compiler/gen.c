char* cdecl_name(Type* type) {
    switch (type->type) {
    case TYPE_VOID:
        return "void";
    case TYPE_INT:
        return "int";
    case TYPE_CHAR:
        return "char";
    case TYPE_FLOAT:
        return "float";
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_ENUM:
        return type->entity->name;
    default:
        assert(0);
        return 0;
    }
}

char* type_to_cdecl(Type* type, char* name) {
    switch (type->type) {
    case TYPE_INT:
        return strf(*name == 0 ? "int" : "int %s", name);
    case TYPE_VOID:
        return strf(*name == 0 ? "void" : "void %s", name);
    case TYPE_CHAR:
        return strf(*name == 0 ? "char" : "char %s", name);
    case TYPE_FLOAT:
        return strf(*name == 0 ? "float" : "float %s", name);
    case TYPE_PTR:
        return type_to_cdecl(type->ptr.base, strf("*%s", name));
    case TYPE_ARRAY:
        return type_to_cdecl(type->array.base, strf("%s[%d]", name, type->array.size)); 
    case TYPE_FUNC: {
        char* buf = NULL;
        buf_printf(buf, "(*%s)(", name);
        for (size_t i = 0; i < type->func.num_params; i++) {
            buf_printf(buf, strf("%s%s", type_to_cdecl(type->func.params[i], ""), i == type->func.num_params - 1 ? "" : ", "));
        }
        buf_printf(buf, ")");
        return type_to_cdecl(type->func.ret, buf);
    }
    case TYPE_STRUCT: {
        char* buf = NULL;
        return buf;
    }
    case TYPE_UNION:
    case TYPE_ENUM:
    default:
        assert(0);
        return 0;
    }
}

char* gen_decl(Decl* decl) {
    switch (decl->type) {
    case DECL_ENUM: 
    case DECL_STRUCT:
    case DECL_UNION:
    case DECL_CONST:
    case DECL_VAR:
    case DECL_TYPEDEF:
    case DECL_FUNC:
    default:
        assert(0);
        return 0;
    }
}

#define _CDECL_TEST(t, x) char* cdecl ## t = (x); printf("|%s|\n", cdecl ## t);

void gen_test(void) {
    printf("----- resolve.c -----\n");
    _CDECL_TEST(1, type_to_cdecl(type_int, "a"));
    _CDECL_TEST(2, type_to_cdecl(type_char, "b"));
    _CDECL_TEST(3, type_to_cdecl(type_float, "c"));
    _CDECL_TEST(4, type_to_cdecl(type_ptr(type_int), "d"));
    _CDECL_TEST(5, type_to_cdecl(type_array(type_func(1, (Type*[]) { type_int }, type_int), 4), "e"));
    _CDECL_TEST(6, type_to_cdecl(type_func(3, (Type*[]) { type_int, type_ptr(type_float), type_array(type_char, 5) }, type_int), "f"));
    _CDECL_TEST(7, type_to_cdecl(type_func(1, (Type*[]) { type_void }, type_void), "g"));
    _CDECL_TEST(8, type_to_cdecl(
        type_func(
            2, 
            (Type*[]) {
                type_int, 
                type_array(
                    type_array(
                        type_func(
                            1, 
                            (Type*[]) { type_char }, 
                            type_ptr(type_float)), 
                        3),
                    5)
            },
            type_ptr(type_char)
        ),
        "h")
    );
    printf("resolve test passed");
}   

#undef _CDECL_TEST
