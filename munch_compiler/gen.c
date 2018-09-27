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

char* gen_op(TokenType op) {
    switch (op) {
    case TOKEN_INC:
        return strf("++");
    case TOKEN_DEC:
        return strf("--");
    case TOKEN_LOG_AND:
        return strf("&&");
    case TOKEN_LOG_OR:
        return strf("||");
    case TOKEN_LSHIFT:
        return strf("<<");
    case TOKEN_RSHIFT:
        return strf(">>");
    case TOKEN_EQ:
        return strf("==");
    case TOKEN_NEQ:
        return strf("!=");
    case TOKEN_LTEQ:
        return strf("<=");
    case TOKEN_GTEQ:
        return strf(">=");
    case TOKEN_COLON_ASSIGN:
        return strf(":=");
    case TOKEN_ADD_ASSIGN:
        return strf("+=");
    case TOKEN_SUB_ASSIGN:
        return strf("-=");
    case TOKEN_DIV_ASSIGN:
        return strf("/=");
    case TOKEN_MUL_ASSIGN:
        return strf("*=");
    case TOKEN_MOD_ASSIGN:
        return strf("%=");
    case TOKEN_BIT_AND_ASSIGN:
        return strf("&=");
    case TOKEN_BIT_OR_ASSIGN:
        return strf("|=");
    case TOKEN_BIT_XOR_ASSIGN:
        return strf("^=");
    case TOKEN_LSHIFT_ASSIGN:
        return strf("<<=");
    case TOKEN_RSHIFT_ASSIGN:
        return strf(">>=");
    case '=': case '!': case ':': case '/': case '*': case '%':
    case '^': case '+': case '-': case '&': case '|': case '<': case '>':
        return strf("%c", (char)op);
    default:
        assert(0);
        return NULL;
    }
}

char* gen_expr(Expr* expr) {
    switch (expr->type) {
    case EXPR_TERNARY:
        return strf("(%s) ? (%s) : (%s)", gen_expr(expr->ternary_expr.cond), gen_expr(expr->ternary_expr.left), gen_expr(expr->ternary_expr.right));
    case EXPR_BINARY:
        return strf("(%s) %s (%s)", gen_expr(expr->binary_expr.left), gen_op(expr->binary_expr.op), gen_expr(expr->binary_expr.right));
    case EXPR_PRE_UNARY:
        return strf("%s(%s)", gen_op(expr->pre_unary_expr.op), gen_expr(expr->pre_unary_expr.expr));
    case EXPR_POST_UNARY:
        return strf("(%s)%s", gen_expr(expr->post_unary_expr.expr), gen_op(expr->post_unary_expr.op));
    case EXPR_CALL: {
        char* buf = NULL;
        buf_printf(buf, "(%s)(", expr->call_expr.expr);
        for (size_t i = 0; i < expr->call_expr.num_args; i++) {
            buf_printf(buf, i == expr->call_expr.num_args - 1 ? "%s" : "%s, ", expr->call_expr.args[i]);
        }
        buf_printf(buf, ")");
        return buf;
    }
    case EXPR_INT:
        return strf("%d", expr->int_expr.int_val);
    case EXPR_FLOAT:
        return strf("%f", expr->float_expr.float_val);
    case EXPR_STR:
#pragma TODO("Generate c expr for string literals");
        return strf("\"%s\"", expr->str_expr.str_val);
    case EXPR_NAME:
        return strf("%s", expr->name_expr.name);
    case EXPR_COMPOUND: {
        char* buf = NULL;
        if (expr->compound_expr.type) {
            buf_printf(buf, "(%s) ", type_to_cdecl(expr->compound_expr.type->resolved_type, ""));
        }
        buf_printf(buf, "{");
        for (size_t i = 0; i < expr->compound_expr.num_compound_items; i++) {
            CompoundItem item = expr->compound_expr.compound_items[i];
            switch (item.type) {
            case COMPOUND_DEFAULT:
                buf_printf(buf, "%s", gen_expr(item.value));
            case COMPOUND_INDEX:
                buf_printf(buf, "[%s] = %s", gen_expr(item.index), gen_expr(item.value));
            case COMPOUND_NAME:
                buf_printf(buf, ".%s = %s", item.name, gen_expr(item.value));
            default:
                assert(0);
            }
            buf_printf(buf, i == expr->compound_expr.num_compound_items - 1 ? "": ", ");
        }
        buf_printf(buf, "}");
        return buf;
    }
    case EXPR_CAST:
        return strf("(%s)(%s)", type_to_cdecl(expr->cast_expr.cast_type->type, ""), gen_expr(expr->cast_expr.cast_expr));
    case EXPR_INDEX:
        return strf("(%s)[%s]", gen_expr(expr->index_expr.expr), gen_expr(expr->index_expr.index));
    case EXPR_FIELD:
        return strf("(%s).%s", gen_expr(expr->field_expr.expr), expr->field_expr.field);
    case EXPR_SIZEOF_TYPE:
        return strf("sizeof(%s)", type_to_cdecl(expr->sizeof_expr.type, ""));
    case EXPR_SIZEOF_EXPR:
        return strf("sizeof(%s)", gen_expr(expr->sizeof_expr.expr));
    default:
        assert(0);
        return NULL;
    }
}

void gen_stmnt(Stmnt* stmnt);

void gen_stmnt_block(BlockStmnt block) {
    genf("{");
    GEN_INDENT;
    for (size_t i = 0; i < block.num_stmnts; i++) {
        gen_stmnt(block.stmnts[i]);
    }
    GEN_UNINDENT;
    genfln("}");
}

void gen_decl_definition(Entity* entity);

void gen_stmnt(Stmnt* stmnt) {
    switch (stmnt->type) {
    case STMNT_DECL:
        // gen_decl_definition();
        break;
    case STMNT_RETURN:
        break;
    case STMNT_IF_ELSE:
        break;
    case STMNT_SWITCH:
        break;
    case STMNT_WHILE:
        break;
    case STMNT_DO_WHILE:
        break;
    case STMNT_FOR:
        break;
    case STMNT_ASSIGN:
        break;
    case STMNT_BREAK:
        break;
    case STMNT_CONTINUE:
        break;
    case STMNT_BLOCK:
        break;
    case STMNT_EXPR:
        break;
    default:
        assert(0);
        break;
    }
}

void gen_decl_definition(Entity* entity) {
    Decl* decl = entity->decl;
    if (decl) {
        switch (decl->type) {
        // case DECL_ENUM:
        case DECL_STRUCT:
        case DECL_UNION: {
            genf("%s %s {", decl->type == DECL_STRUCT ? "struct" : "union", entity->name);
            GEN_INDENT;
            Type* type = entity->type;
            for (size_t i = 0; i < type->aggregate.num_fields; i++) {
                genfln("%s;", type_to_cdecl(type->aggregate.fields[i].type, type->aggregate.fields[i].name));
            }
            GEN_UNINDENT;
            genfln("};");
            break;
        }
        case DECL_CONST:
            genfln("const %s = %s;", type_to_cdecl(entity->type, entity->name), gen_expr(decl->const_decl.expr));
            break;
        case DECL_VAR:
            if (decl->var_decl.expr) {
                genfln("%s = %s;", type_to_cdecl(entity->type, entity->name), gen_expr(decl->var_decl.expr));
            }
            else {
                genfln("%s;", type_to_cdecl(entity->type, entity->name));
            }
            break;
        case DECL_TYPEDEF:
            break;
        case DECL_FUNC:
            genf("%s %s(", type_to_cdecl(decl->func_decl.ret_type->type, ""), decl->name);
            for (size_t i = 0; i < decl->func_decl.num_params; i++) {
                genfln(i == decl->func_decl.num_params - 1 ? "%s" : "%s, ", 
                       type_to_cdecl(decl->func_decl.params[i].type->type, decl->func_decl.params[i].name));
            }
            genf(")");
            gen_stmnt_block(decl->func_decl.block);
            break;
        default:
            return;
        }
    }
    else {

    }
}

void gen_decls(void) {
    for (size_t i = 0; i < buf_len(ordered_entities); i++) {
        gen_decl_definition(ordered_entities[i]);
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
                        "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}\n"
                        "const a = 1 % 3\n";

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
