char* gen_buf = NULL;

#define genf(fmt, ...) \
    do { \
        buf_printf(gen_buf, (fmt), __VA_ARGS__); \
    } while(0)

int gen_indent;
#define ___ "                                              "

void gen_new_line(void) {
    genf("\n%.*s", 2 * gen_indent, ((___ ___ ___ ___)));
}

#undef ___

#define GEN_INDENT gen_indent++; gen_new_line()
#define GEN_UNINDENT gen_indent--; gen_new_line()

#define genfln(fmt, ...) \
    do { \
        buf_printf(gen_buf, (fmt), __VA_ARGS__); \
        gen_new_line(); \
    } while(0)

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
        return strf("%s", type->entity->name);
    default:
        assert(0);
        return 0;
    }
}

char* type_to_cdecl(Type* type, const char* name) {
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
    case TYPE_STRUCT:
    case TYPE_UNION:
        return strf(*name == 0 ? "%s" : "%s %s", type->entity->name, name);
    case TYPE_ENUM:
    default:
        assert(0);
        return 0;
    }
}

void gen_forward_decl(Entity* entity) {
    Decl* decl = entity->decl;
    if (!decl) {
        return;
    }
    switch (decl->type) {
    case DECL_STRUCT:
        genfln("typedef struct %s %s;", decl->name, decl->name);
        break;
    case DECL_UNION:
        genfln("typedef union %s %s;", decl->name, decl->name);
        break;
    case DECL_FUNC:
        genfln("%s;", type_to_cdecl(entity->type, entity->name));
        break;
    case DECL_VAR:
        genfln("%s;", type_to_cdecl(entity->type, entity->name));
        break;
    }
}

void gen_forward_decls(void) {
    for (size_t i = 0; i < buf_len(ordered_entities); i++) {
        gen_forward_decl(ordered_entities[i]);
    }
}

void gen_decl(Entity* entity) {
    Decl* decl = entity->decl;
    if (decl) {
        switch (decl->type) {
        case DECL_ENUM:
        case DECL_STRUCT: 
        case DECL_UNION: {
            genf("%s %s {", decl->type == DECL_STRUCT ? "struct" : "union", entity->name);
            GEN_INDENT;
            Type* type = entity->type;
            for (size_t i = 0; i < type->aggregate.num_fields; i++) {
                genfln("%s;", type_to_cdecl(type->aggregate.fields[i].type, type->aggregate.fields[i].name));
            }
            GEN_UNINDENT;
            genf("};");
            break;
        }
        case DECL_CONST:
        case DECL_VAR:
        case DECL_TYPEDEF:
        case DECL_FUNC:
        default:
            return;
        }
    }
    else {

    }
}

void gen_decls(void) {
    for (size_t i = 0; i < buf_len(ordered_entities); i++) {
        gen_decl(ordered_entities[i]);
    }
}

void gen_all(void) {
    genfln("// Forward declarations");
    gen_forward_decls();
    genfln("");
    genfln("// Defintions");
    gen_decls();
}

#define _CDECL_TEST(t, x) char* cdecl ## t = (x); printf("|%s|\n", cdecl ## t);

void gen_test(void) {
    printf("----- resolve.c -----\n");

#if 0
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
#endif

    const char* src = "struct V {x: int; y: int;}\n"
                        "var v: V = {y=1, x=2}\n"
                        "var w: int[3] = {1, [2]=3}\n"
                        "var vv: V[2] = {{1, 2}, {3, 4}}\n"
                        "func add(a: V, b: V): V { var c: V; c = {a.x + b.x, a.y + b.y}; return c; }\n"
                        "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}\n";

    init_stream(src);
    install_built_in_types();
    install_built_in_consts();
    DeclSet* declset = parse_stream();
    install_decls(declset);
    complete_entities();
    
    gen_all();

    printf("\n\n%s\n\n", gen_buf);

    printf("resolve test passed");
}   

#undef _CDECL_TEST
