char* gen_buf = NULL;

#define genf(fmt, ...) \
    do { \
        buf_printf(gen_buf, (fmt), ##__VA_ARGS__); \
    } while(0)

int gen_indent;
#define ___ "                                                                                       "

void gen_new_line(void) {
    genf("\n%.*s", 2 * gen_indent, ((___ ___ ___ ___)));
}

#undef ___

#define GEN_INDENT gen_indent++; gen_new_line()
#define GEN_UNINDENT gen_indent--; gen_new_line()

#define genfln(fmt, ...) \
    do { \
        buf_printf(gen_buf, (fmt), ##__VA_ARGS__); \
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

char* gen_func_head(Decl* decl) {
    assert(decl->type == DECL_FUNC);
    char* buf = NULL;
    buf_printf(buf, "%s %s(", type_to_cdecl(decl->func_decl.ret_type->resolved_type, ""), decl->name);
    if (decl->func_decl.num_params) {
        for (size_t i = 0; i < decl->func_decl.num_params; i++) {
            buf_printf(buf, i == decl->func_decl.num_params - 1 ? "%s" : "%s, ",
                       type_to_cdecl(decl->func_decl.params[i].type->resolved_type, decl->func_decl.params[i].name));
        }
    }
    else {
        buf_printf(buf, "void");
    }
    buf_printf(buf, ")");
    return buf;
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
        genfln("%s;", gen_func_head(decl));
        break;
    case DECL_TYPEDEF:
        genfln("typedef %s;", type_to_cdecl(entity->type, decl->name));
        break;
    default:
        break;
    }
}

void gen_decls_forward(void) {
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

char* gen_expr_ff(Expr* expr, bool force_fold);
char* gen_expr_core(Expr* expr, bool type_expected, bool force_fold);
void gen_stmnt(Stmnt* stmnt);

char* gen_expr_ternary(Expr* expr, bool force_fold) {
    return strf("(%s) ? (%s) : (%s)", gen_expr_ff(expr->ternary_expr.cond, force_fold), gen_expr_ff(expr->ternary_expr.left, force_fold), gen_expr_ff(expr->ternary_expr.right, force_fold));
}

char* gen_expr_binary(Expr* expr, bool force_fold) {
    return strf("(%s) %s (%s)", gen_expr_ff(expr->binary_expr.left, force_fold), gen_op(expr->binary_expr.op), gen_expr_ff(expr->binary_expr.right, force_fold));
}

char* gen_expr_pre_unary(Expr* expr, bool force_fold) {
    return strf("%s(%s)", gen_op(expr->pre_unary_expr.op), gen_expr_ff(expr->pre_unary_expr.expr, force_fold));
}

char* gen_expr_post_unary(Expr* expr, bool force_fold) {
    return strf("(%s)%s", gen_expr_ff(expr->post_unary_expr.expr, force_fold), gen_op(expr->post_unary_expr.op));
}

char* gen_expr_call(Expr* expr, bool force_fold) {
    char* buf = NULL;
    buf_printf(buf, "(%s)(", gen_expr_ff(expr->call_expr.expr, force_fold));
    for (size_t i = 0; i < expr->call_expr.num_args; i++) {
        buf_printf(buf, i == expr->call_expr.num_args - 1 ? "%s" : "%s, ", gen_expr_ff(expr->call_expr.args[i], force_fold));
    }
    buf_printf(buf, ")");
    return buf;
}

char* gen_expr_int(Expr* expr, bool force_fold) {
    return strf("%d", expr->int_expr.int_val);
}

char* gen_expr_float(Expr* expr, bool force_fold) {
    return strf("%f", expr->float_expr.float_val);
}

char* gen_expr_str(Expr* expr, bool force_fold) {
    char* buf = NULL;
    buf_printf(buf, "\"");
    for(size_t i = 0, s = strlen(expr->str_expr.str_val); i < s; i++) {
        if(esc_char_to_str[(size_t)expr->str_expr.str_val[i]]) {
            buf_printf(buf, "%s", esc_char_to_str[(size_t)expr->str_expr.str_val[i]]);
        } else {
            printf("%c", expr->str_expr.str_val[i]);
        }
    }
    buf_printf(buf, "\"");
    return buf;
}

char* gen_expr_name(Expr* expr, bool force_fold) {
    if (force_fold) {
        Decl* decl = get_entity(expr->name_expr.name)->decl;
        Expr* decl_expr = NULL;
        if (decl->type == DECL_VAR) {
            decl_expr = decl->var_decl.expr;
        }
        else if (decl->type == DECL_CONST) {
            decl_expr = decl->const_decl.expr;
        }
        else {
            assert(0);
        }
        return gen_expr_ff(decl_expr, true);
    }
    return strf("%s", expr->name_expr.name);
}

char* gen_expr_compound(Expr* expr, bool type_expected, bool force_fold) {
    char* buf = NULL;
    if (force_fold) {
        type_expected = false;
    }
    if (type_expected) {
        buf_printf(buf, "(%s) ", type_to_cdecl(expr->resolved_type, ""));
    }
    buf_printf(buf, "{");
    for (size_t i = 0; i < expr->compound_expr.num_compound_items; i++) {
        CompoundItem item = expr->compound_expr.compound_items[i];
        switch (item.type) {
        case COMPOUND_DEFAULT:
            buf_printf(buf, "%s", gen_expr_core(item.value, type_expected, force_fold));
            break;
        case COMPOUND_INDEX:
            buf_printf(buf, "[%s] = %s", gen_expr_ff(item.index, force_fold), gen_expr_core(item.value, type_expected, force_fold));
            break;
        case COMPOUND_NAME:
            buf_printf(buf, ".%s = %s", item.name, gen_expr_core(item.value, type_expected, force_fold));
            break;
        default:
            assert(0);
        }
        buf_printf(buf, i == expr->compound_expr.num_compound_items - 1 ? "" : ", ");
    }
    buf_printf(buf, "}");
    return buf;
}

char* gen_expr_cast(Expr* expr, bool force_fold) {
    return strf("(%s)(%s)", type_to_cdecl(expr->cast_expr.cast_type->resolved_type, ""), gen_expr_ff(expr->cast_expr.cast_expr, force_fold));
}

char* gen_expr_index(Expr* expr, bool force_fold) {
    return strf("(%s)[%s]", gen_expr_ff(expr->index_expr.expr, force_fold), gen_expr_ff(expr->index_expr.index, force_fold));
}

char* gen_expr_field(Expr* expr, bool force_fold) {
    return strf("(%s).%s", gen_expr_ff(expr->field_expr.expr, force_fold), expr->field_expr.field);
}

char* gen_expr_sizeof_type(Expr* expr, bool force_fold) {
    return strf("sizeof(%s)", type_to_cdecl(expr->sizeof_expr.type->resolved_type, ""));
}

char* gen_expr_sizeof_expr(Expr* expr, bool force_fold) {
    return strf("sizeof(%s)", gen_expr_ff(expr->sizeof_expr.expr, force_fold));
}

char* gen_expr_core(Expr* expr, bool type_expected, bool force_fold) {
    if (expr->is_folded && expr->resolved_type == type_int) {
        return strf("%d", expr->resolved_value);
    }
    switch (expr->type) {
    case EXPR_TERNARY:
        return gen_expr_ternary(expr, force_fold);
    case EXPR_BINARY:
        return gen_expr_binary(expr, force_fold);
    case EXPR_PRE_UNARY:
        return gen_expr_pre_unary(expr, force_fold);
    case EXPR_POST_UNARY:
        return gen_expr_post_unary(expr, force_fold);
    case EXPR_CALL: 
        return gen_expr_call(expr, force_fold);
    case EXPR_INT:
        return gen_expr_int(expr, force_fold);
    case EXPR_FLOAT:
        return gen_expr_float(expr, force_fold);
    case EXPR_STR:
        return gen_expr_str(expr, force_fold);
    case EXPR_NAME:
        return gen_expr_name(expr, force_fold);
    case EXPR_COMPOUND:
        return gen_expr_compound(expr, type_expected, force_fold);
    case EXPR_CAST:
        return gen_expr_cast(expr, force_fold);
    case EXPR_INDEX:
        return gen_expr_index(expr, force_fold);
    case EXPR_FIELD:
        return gen_expr_field(expr, force_fold);
    case EXPR_SIZEOF_TYPE:
        return gen_expr_sizeof_type(expr, force_fold);
    case EXPR_SIZEOF_EXPR:
        return gen_expr_sizeof_expr(expr, force_fold);
    default:
        assert(0);
        return NULL;
    }
}

char* gen_expr_ff(Expr* expr, bool force_fold) {
    return gen_expr_core(expr, true, force_fold);
}

char* gen_expr(Expr* expr) {
    return gen_expr_core(expr, true, false);
}

void gen_stmnt_block(BlockStmnt block) {
    genf("{");
    GEN_INDENT;
    for (size_t i = 0; i < block.num_stmnts; i++) {
        if (block.stmnts[i]) {
            gen_stmnt(block.stmnts[i]);
            genf(";");
            if (i != block.num_stmnts - 1) genfln("");
        }
    }
    GEN_UNINDENT;
    genf("}");
}

void gen_decl_def(Entity* entity);

void gen_stmnt_decl(Stmnt* stmnt) {
    assert(stmnt->decl_stmnt.decl->type == DECL_VAR);
    Decl* decl = stmnt->decl_stmnt.decl;
    genf("%s", type_to_cdecl(decl->var_decl.expr->resolved_type, decl->name));
    if (decl->var_decl.expr) {
        genf(" = %s", gen_expr(decl->var_decl.expr));
    }
}

void gen_stmnt_return(Stmnt* stmnt) {
    if (stmnt->return_stmnt.expr) {
        genf("return %s", gen_expr(stmnt->return_stmnt.expr));
    }
    else {
        genf("return");
    }
}

void gen_stmnt_ifelse(Stmnt* stmnt) {
    genf("if (%s) ", gen_expr(stmnt->ifelseif_stmnt.if_cond));
    gen_stmnt_block(stmnt->ifelseif_stmnt.then_block);
    for (size_t i = 0; i < stmnt->ifelseif_stmnt.num_else_ifs; i++) {
        genf("else if(%s) ", gen_expr(stmnt->ifelseif_stmnt.else_ifs[i].cond));
        gen_stmnt_block(stmnt->ifelseif_stmnt.else_ifs[i].block);
    }
    if (stmnt->ifelseif_stmnt.else_block.num_stmnts) {
        genf("else ");
        gen_stmnt_block(stmnt->ifelseif_stmnt.else_block);
    }
}

void gen_stmnt_switch(Stmnt* stmnt) {
    genf("switch (%s) {", gen_expr(stmnt->switch_stmnt.switch_expr));
    genfln("");
    for (size_t i = 0; i < stmnt->switch_stmnt.num_case_blocks; i++) {
        genf("case %s: ", gen_expr(stmnt->switch_stmnt.case_blocks[i].case_expr));
        gen_stmnt_block(stmnt->switch_stmnt.case_blocks[i].block);
    }
    if (stmnt->switch_stmnt.default_block.num_stmnts) {
        genf("default: ");
        gen_stmnt_block(stmnt->switch_stmnt.default_block);
    }
    genfln("}");
}

void gen_stmnt_while(Stmnt* stmnt) {
    genf("while (%s) ", gen_expr(stmnt->while_stmnt.cond));
    gen_stmnt_block(stmnt->while_stmnt.block);
}

void gen_stmnt_do_while(Stmnt* stmnt) {
    genf("do ");
    gen_stmnt_block(stmnt->while_stmnt.block);
    genf(" while (%s)", gen_expr(stmnt->while_stmnt.cond));
}

void gen_stmnt_for(Stmnt* stmnt) {
    genf("for (");
    for (size_t i = 0; i < stmnt->for_stmnt.num_init; i++) {
        gen_stmnt(stmnt->for_stmnt.init[i]);
        if (i != stmnt->for_stmnt.num_init - 1) genf(", ");
    }
    genf(stmnt->for_stmnt.cond ? "; %s; " : "; ; ", gen_expr(stmnt->for_stmnt.cond));
    for (size_t i = 0; i < stmnt->for_stmnt.num_update; i++) {
        gen_stmnt(stmnt->for_stmnt.update[i]);
        if (i != stmnt->for_stmnt.num_update - 1) genf(", ");
    }
    genf(")");
    gen_stmnt_block(stmnt->for_stmnt.block);
}

void gen_stmnt_assign(Stmnt* stmnt) {
    genf("%s %s %s", gen_expr(stmnt->assign_stmnt.left), gen_op(stmnt->assign_stmnt.op), gen_expr(stmnt->assign_stmnt.right));
}

void gen_stmnt_init(Stmnt* stmnt) {
    genf("%s = %s", type_to_cdecl(stmnt->init_stmnt.right->resolved_type, stmnt->init_stmnt.left->name_expr.name), gen_expr(stmnt->init_stmnt.right));
}

void gen_stmnt_break(Stmnt* stmnt) {
    genf("break");
}

void gen_stmnt_continue(Stmnt* stmnt) {
    genf("continue");
}

void gen_stmnt_expr(Stmnt* stmnt) {
    genf("%s", gen_expr(stmnt->expr_stmnt.expr));
}

void gen_stmnt(Stmnt* stmnt) {
    if (!stmnt) {
        return;
    }
    switch (stmnt->type) {
    case STMNT_DECL:
        gen_stmnt_decl(stmnt);
        break;
    case STMNT_RETURN:
        gen_stmnt_return(stmnt);
        break;
    case STMNT_IF_ELSE:
        gen_stmnt_ifelse(stmnt);
        break;
    case STMNT_SWITCH:
        gen_stmnt_switch(stmnt);
        break;
    case STMNT_WHILE:
        gen_stmnt_while(stmnt);
        break;
    case STMNT_DO_WHILE:
        gen_stmnt_do_while(stmnt);
        break;
    case STMNT_FOR:
        gen_stmnt_for(stmnt);
        break;
    case STMNT_ASSIGN:
        gen_stmnt_assign(stmnt);
        break;
    case STMNT_INIT:
        gen_stmnt_init(stmnt);
        break;
    case STMNT_BREAK:
        gen_stmnt_break(stmnt);
        break;
    case STMNT_CONTINUE:
        gen_stmnt_continue(stmnt);
        break;
    case STMNT_BLOCK:
        gen_stmnt_block(stmnt->block_stmnt);
        break;
    case STMNT_EXPR:
        gen_stmnt_expr(stmnt);
        break;
    default:
        assert(0);
        break;
    }
}

void gen_decl_def_aggregate(Entity* entity) {
    Decl* decl = entity->decl;
    genf("%s %s {", decl->type == DECL_STRUCT ? "struct" : "union", entity->name);
    GEN_INDENT;
    Type* type = entity->type;
    for (size_t i = 0; i < type->aggregate.num_fields; i++) {
        genf("%s;", type_to_cdecl(type->aggregate.fields[i].type, type->aggregate.fields[i].name));
        if (i != type->aggregate.num_fields - 1) genfln("");
    }
    GEN_UNINDENT;
    genf("};");
}

void gen_decl_def_const(Entity* entity) {
    if (entity->type == type_int) {
        genf("const %s = %d;", type_to_cdecl(entity->type, entity->name), entity->value);
    }
    else {
        genf("const %s = %s;", type_to_cdecl(entity->type, entity->name), gen_expr(entity->decl->const_decl.expr));
    }
}

void gen_decl_def_var(Entity* entity) {
    Decl* decl = entity->decl;
    if (decl->var_decl.expr) {
        genf("%s = %s;", type_to_cdecl(entity->type, entity->name), gen_expr_core(decl->var_decl.expr, false, true));
    }
    else {
        genf("%s;", type_to_cdecl(entity->type, entity->name));
    }
}

void gen_decl_def_func(Entity* entity) {
    genf("%s ", gen_func_head(entity->decl));
    gen_stmnt_block(entity->decl->func_decl.block);
}

void gen_decl_def_enum_const(Entity* entity) {
    genf("enum { %s = %d };", entity->name, entity->value);
}

void gen_decl_def(Entity* entity) {
    if (entity->e_type == ENTITY_ENUM_CONST) {
        gen_decl_def_enum_const(entity);
        genfln("");
        return;
    }
    Decl* decl = entity->decl;
    if (decl) {
        switch (decl->type) {
        case DECL_NONE:
        case DECL_ENUM:
        case DECL_TYPEDEF:
            break;
        case DECL_STRUCT:
        case DECL_UNION: 
            gen_decl_def_aggregate(entity);
            genfln("");
            break;
        case DECL_CONST:
            gen_decl_def_const(entity);
            genfln("");
            break;
        case DECL_VAR:
            gen_decl_def_var(entity);
            genfln("");
            break;
        case DECL_FUNC:
            gen_decl_def_func(entity);
            genfln("");
            break;
        }
    }
}

void gen_decls_def(void) {
    Entity** func_entities = NULL;
    for (size_t i = 0; i < buf_len(ordered_entities); i++) {
        if (ordered_entities[i]->e_type == ENTITY_FUNC) {
            buf_push(func_entities, ordered_entities[i]);
        }
        else {
            gen_decl_def(ordered_entities[i]);
        }
    }
    for (size_t i = 0; i < buf_len(func_entities); i++) {
        gen_decl_def(func_entities[i]);
    }
}

void gen_all(void) {
    gen_buf = NULL;
    genfln("// Forward declarations");
    gen_decls_forward();
    genfln("");
    genfln("// Defintions");
    gen_decls_def();
}

void gen_buf_to_file(const char* path) {
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s", gen_buf);
        fclose(f);
    }
    else {
        fatal("Error opening file at %s", path);
    }
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
        "func add(a: V, b: V): V { c := V{0}; c = {a.x + b.x, a.y + b.y}; return c; }\n"
        "func fib(n: int): int { if(n <= 1) {return n;} return fib(n - 1) + fib(n - 2);}\n"
        "const a = 1 % 3\n"
        "func main():int {}";

    init_stream(src);
    install_built_in_types();
    install_built_in_consts();
    DeclSet* declset = parse_stream();
    install_decls(declset);
    complete_entities();
    
    gen_all();
    gen_buf_to_file("output\\munch_output.c");

    printf("\n\n%s\n\n", gen_buf);

    printf("resolve test passed");
}   

#undef _CDECL_TEST
