#pragma TODO("PARSE ALL!!!")

Stmnt* parse_stmnt();

const char* parse_name() {
    const char* name = token.name;
    expect_token(TOKEN_NAME);
    return name;
}

TypeSpec* parse_typespec() {
    printf("parse typespec\n");
    return NULL;
}

Expr* parse_expr() {
    printf("parse expr");
    return NULL;
}

Stmnt* parse_stmnt_ifelseif() {
    return NULL;
}

Stmnt* parse_stmnt_switch() {
    return NULL;
}

Stmnt* parse_stmnt_while() {
    return NULL;
}

Stmnt* parse_stmnt_do() {
    return NULL;
}

Stmnt* parse_stmnt_for() {
    return NULL;
}

Stmnt* parse_stmnt_assign() {
    return NULL;
}

Stmnt* parse_stmnt_return() {
    return stmnt_return(parse_expr());
}

BlockStmnt parse_blockstmnt() {
    return (BlockStmnt) { 0, NULL };
}

Stmnt* parse_stmnt_block() {
    expect_token('{');
    Stmnt** stmnts = NULL;
    while (!is_token('}')) {
        buf_push(stmnts, parse_stmnt());
    }
    expect_token('}');
    return stmnt_block(buf_len(stmnts), ast_dup(stmnts, sizeof(Stmnt*) * buf_len(stmnts)));
}

Stmnt* parse_stmnt_expr() {
    return stmnt_expr(parse_expr());
}

Stmnt* parse_stmnt() {
    if (match_keyword(kwrd_if)) {
        return parse_stmnt_ifelseif();
    }
    else if (match_keyword(kwrd_switch)) {
        return parse_stmnt_switch();
    }
    else if (match_keyword(kwrd_while)) {
        return parse_stmnt_while();
    }
    else if (match_keyword(kwrd_do)) {
        return parse_stmnt_do();
    }
    else if (match_keyword(kwrd_for)) {
        return parse_stmnt_for();
    }
    else if (match_keyword(kwrd_return)) {
        return parse_stmnt_return();
    }
    else if (is_token('{')) {
        return parse_stmnt_block();
    }
    else if (match_keyword(kwrd_break)) {
        return stmnt_break();
    }
    else if (match_keyword(kwrd_continue)) {
        return stmnt_continue();
    }
    return NULL;
}

EnumItem parse_enum_item() {
    const char* name = parse_name();
    if (match_token('=')) {
        return (EnumItem) { name, parse_expr() };
    }
    return (EnumItem) { name, NULL };
}

Decl* parse_decl_enum() {
    const char* name = parse_name();
    expect_token('{');
    EnumItem* enum_items = NULL;
    buf_push(enum_items, parse_enum_item());
    while (match_token(',')) {
        buf_push(enum_items, parse_enum_item());
    }
    expect_token('}');
    return decl_enum(name, buf_len(enum_items), enum_items);
}

AggregateItem parse_aggregate_item() {
    const char* name = parse_name();
    if (match_token(':')) {
        TypeSpec* type = parse_typespec();
        return match_token('=') ? (AggregateItem){ name, type, NULL } 
            : (AggregateItem) { name, type, parse_expr() };
    }
    expect_token('=');
    return (AggregateItem) { name, NULL, parse_expr() };
}

Decl* parse_aggregate(DeclType type) {
    assert(type == DECL_STRUCT || type == DECL_UNION);
    const char* name = parse_name();
    expect_token('{');
    AggregateItem* aggregate_items = NULL;
    buf_push(aggregate_items, parse_aggregate_item());
    while (match_token(',')) {
        buf_push(aggregate_items, parse_aggregate_item());
    }
    expect_token('}');
    return decl_aggregate(type, name, buf_len(aggregate_items), aggregate_items);
}

Decl* parse_decl_struct() {
    return parse_aggregate(DECL_STRUCT);
}

Decl* parse_decl_union() {
    return parse_aggregate(DECL_UNION);
}

Decl* parse_decl_const() {
    const char* name = parse_name();
    expect_token('=');
    return decl_const(name, parse_expr());
}

Decl* parse_decl_var() {
    const char* name = parse_name();
    if (match_token(':')) {
        TypeSpec* type = parse_typespec();
        return match_token('=') ? decl_var(name, type, NULL)
            : decl_var(name, type, parse_expr());
    }
    else if (match_token('=')) {
        return decl_var(name, NULL, parse_expr());
    }
    else {
        syntax_error("Expected = or : in var declaration. Got %s", get_token_type_name(token.type));
    }
}

Decl* parse_decl_typedef() {
    const char* name = parse_name();
    expect_token('=');
    return decl_typedef(name, parse_typespec());
}

FuncParam parse_func_param() {
    const char* name = parse_name();
    TypeSpec* type = parse_typespec();
    return (FuncParam) { name, type };
}

Decl* parse_decl_func() {
    const char* name = parse_name();
    expect_token('(');
    FuncParam* func_params = NULL;
    if (!is_token(')')) {
        buf_push(func_params, parse_func_param());
        while (match_token(',')) {
            buf_push(func_params, parse_func_param());
        }
    }
    expect_token(')');
    TypeSpec* ret_type = NULL;
    if (is_token(':')) {
        ret_type = parse_typespec();
    }
    return decl_func(name, buf_len(func_params), func_params, ret_type, parse_blockstmnt());
}

Decl* parse_decl() {
    if (match_keyword(kwrd_enum)) {
        return parse_decl_enum();
    }
    else if (match_keyword(kwrd_struct)) {
        return parse_decl_struct();
    }
    else if (match_keyword(kwrd_union)) {
        return parse_decl_union();
    } 
    else if (match_keyword(kwrd_const)) {
        return parse_decl_const();
    }
    else if (match_keyword(kwrd_var)) {
        return parse_decl_var();
    }
    else if (match_keyword(kwrd_typedef)) {
        return parse_decl_typedef();
    }
    else if (match_keyword(kwrd_func)) {
        return parse_decl_func();
    }
    syntax_error("Expected a declaration keyword. Found %s", get_token_type_name(token.type));
}

parse_test() {
    printf("----- parse.c -----\n");
    
    init_keywords();

    init_stream("const pi = 3.14");
    Decl* d1 = parse_decl();
    print_decl(d1);

    printf("parse test passed\n");
}