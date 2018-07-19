#pragma TODO("PARSE ALL!!!")

const char* parse_name() {
    const char* name = token.name;
    expect_token(TOKEN_NAME);
    return name;
}

TypeSpec* parse_typespec() {
    printf("parse typespec\n");
    return NULL;
}

Stmnt* parse_stmnt() {
    printf("parse stmnt\n");
    return NULL;
}

Expr* parse_expr() {

    return NULL;
}

Decl* parse_decl_const() {
    const char* name = parse_name();
    expect_token('=');
    return decl_const(name, parse_expr());
}

Decl* parse_decl() {
    if (match_keyword(kwrd_const)) {
        return parse_decl_const();
    }
    return NULL;
}

parse_test() {
    printf("----- parse.c -----\n");
    
    init_keywords();

    init_stream("const pi = 3.14");
    Decl* d1 = parse_decl();
    print_decl(d1);

    printf("parse test passed\n");
}