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

Expr* parse_expr_literal() {
    if (token.type == TOKEN_INT) {
        uint64_t intval = token.intval;
        next_token();
        return expr_int(intval);
    }
    else if (token.type == TOKEN_FLOAT) {
        double floatval = token.floatval;
        next_token();
        return expr_float(floatval);
    }
    else if (token.type == TOKEN_STR) {
        const char* strval = token.strval;
        next_token();
        return expr_str(strval);
    }
    syntax_error("Expected a literal. Found <ASCII %d>", token.type);
}

Expr* parse_expr_compound(TypeSpec* type) {
    expect_token('{');
    Expr** exprs = NULL;
    if (!is_token('}')) {
        buf_push(exprs, parse_expr());
        while (match_token(',')) {
            buf_push(exprs, parse_expr());
        }
    }
    expect_token('}');
    return expr_compound(type, buf_len(exprs), exprs);
}

Expr* parse_expr_operand() {
    if (is_literal()) {
        return parse_expr_literal();
    }
    else if (is_token('{')) {
        return parse_expr_compound(NULL);
    }
    else if (is_token(TOKEN_NAME)) {
        const char* name = parse_name();
        if (is_token('{')) {
            return parse_expr_compound(typespec_name(name));
        }
        return expr_name(name);
    }
    else if (match_token('(')) {
        if (is_token(':')) {
            TypeSpec* type = parse_typespec();
            expect_token(')');
            return parse_expr_compound(type);
        }
        else {
            Expr* expr = parse_expr();
            expect_token(')');
            return parse_expr();
        }
    }
    syntax_error("Expected an operand expression. Found <ASCII %d>", token.type);
}

Expr* parse_expr_base() {
    Expr* operand_expr = parse_expr_operand();
    while (true) {
        if (match_token('(')) {
            Expr** args = NULL;
            if (!is_token(')')) {
                buf_push(args, parse_expr());
                while (match_token(',')) {
                    buf_push(args, parse_expr());
                }
            }
            expect_token(')');
            operand_expr = expr_call(operand_expr, buf_len(args), args);
        }
        else if (match_token('[')) {
            operand_expr = expr_index(operand_expr, parse_expr());
            expect_token(']');
        }
        else if (match_token('.')) {
            operand_expr = expr_field(operand_expr, parse_name());
        }
        else {
            break;
        }
    }
    return operand_expr;
}

Expr* parse_expr_unary() {
    if (is_unary_op()) {
        TokenType unary_op = token.type;
        next_token();
        return expr_unary(unary_op, parse_expr_base());
    }
    return parse_expr_base();
}

Expr* parse_expr_mul() {
    Expr* unary_expr = parse_expr_unary();
    while (is_mul_op()) {
        TokenType mul_op = token.type;
        next_token();
        unary_expr = expr_binary(mul_op, unary_expr, parse_expr_unary());
    }
    return unary_expr;
}

Expr* parse_expr_add() {
    Expr* mul_expr = parse_expr_mul();
    while (is_add_op()) {
        TokenType add_op = token.type;
        next_token();
        mul_expr = expr_binary(add_op, mul_expr, parse_expr_cmp());
    }
    return mul_expr;
}

Expr* parse_expr_shift() {
    Expr* add_expr = parse_expr_add();
    if (is_shift_op()) {
        TokenType shift_op = token.type;
        next_token();
        add_expr = expr_binary(shift_op, add_expr, parse_expr_add());
    }
    return add_expr;
}

Expr* parse_expr_cmp() {
    Expr* shift_expr = parse_expr_shift();
    if (is_cmp_op()) {
        TokenType cmp_op = token.type;
        next_token();
        shift_expr = expr_binary(cmp_op, shift_expr, parse_expr_shift());
    }
    return shift_expr;
}

Expr* parse_expr_bit_xor() {
    Expr* cmp_expr = parse_expr_cmp();
    while (match_token('^')) {
        cmp_expr = expr_binary('^', cmp_expr, parse_expr_cmp());
    }
    return cmp_expr;
}

Expr* parse_expr_bit_and() {
    Expr* bit_xor_expr = parse_expr_bit_xor();
    while (match_token('&')) {
        bit_xor_expr = expr_binary('&', bit_xor_expr, parse_expr_bit_xor());
    }
    return bit_xor_expr;
}

Expr* parse_expr_bit_or() {
    Expr* bit_and_expr = parse_expr_bit_and();
    while (match_token('|')) {
        bit_and_expr = expr_binary('|', bit_and_expr, parse_expr_bit_and());
    }
    return bit_and_expr;
}

Expr* parse_expr_and() {
    Expr* bit_or_expr = parse_expr_bit_or();
    while (match_token(TOKEN_LOG_AND)) {
        bit_or_expr = expr_binary(TOKEN_LOG_AND, bit_or_expr, parse_expr_bit_or());
    }
    return bit_or_expr;
}

Expr* parse_expr_or() {
    Expr* and_expr = parse_expr_and();
    while (match_token(TOKEN_LOG_OR)) {
        and_expr = expr_binary(TOKEN_LOG_OR, and_expr, parse_expr_and());
    }
    return and_expr;
}

Expr* parse_expr_ternary() {
    Expr* or_expr = parse_expr_or();
    if (match_token('?')) {
        Expr* left = parse_expr_ternary();
        expect_token(':');
        Expr* right = parse_expr_ternary();
        return expr_ternary(or_expr, left, right);
    }
    return or_expr;
}

Expr* parse_expr() {
    return parse_expr_ternary();
}

Stmnt* parse_stmnt_ifelseif() {
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    BlockStmnt then_block = parse_blockstmnt();
    ElseIfItem* else_ifs = NULL;
    while (is_keyword(kwrd_else) && is_keyword(kwrd_if)) {
        next_token();
        next_token();
        expect_token('(');
        Expr* elseif_cond = parse_expr();
        expect_token(')');
        ElseIfItem elseif_item = (ElseIfItem) { elseif_cond, parse_blockstmnt() };
        buf_push(else_ifs, elseif_item);
    }
    BlockStmnt else_block = (BlockStmnt) { 0, NULL };
    if (match_keyword(kwrd_else)) {
        else_block = parse_blockstmnt();
    }
    return stmnt_ifelseif(cond, then_block, buf_len(else_ifs), else_ifs, else_block);
}

CaseBlock parse_case_block() {
    expect_keyword(kwrd_case);
    Expr* case_expr = parse_expr();
    expect_token(':');
    return (CaseBlock) { case_expr, parse_blockstmnt() };
}

Stmnt* parse_stmnt_switch() {
    expect_token('(');
    Expr* switch_expr = parse_expr();
    expect_token(')');
    CaseBlock* case_blocks = NULL;
    BlockStmnt default_block = (BlockStmnt) { 0, NULL };
    expect_token('{');
    if (!is_token('}')) {
        if (!is_keyword(kwrd_default)) {
            while (is_keyword(kwrd_case)) {
                buf_push(case_blocks, parse_case_block());
            }
        }
        if (match_keyword(kwrd_default)) {
            expect_token(':');
            default_block = parse_blockstmnt();
        }
    }
    expect_token('}');
    return stmnt_switch(switch_expr, buf_len(case_blocks), case_blocks, default_block);
}

Stmnt* parse_stmnt_while() {
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    return stmnt_while(STMNT_WHILE, cond, parse_blockstmnt());
}

Stmnt* parse_stmnt_do() {
    BlockStmnt block = parse_blockstmnt();
    expect_keyword(kwrd_while);
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    return stmnt_while(STMNT_DO_WHILE, cond, block);
}

Stmnt* parse_for_init_update() {
    Stmnt* stmnt = parse_stmnt();
    if (stmnt->type != STMNT_ASSIGN && stmnt->type != STMNT_EXPR) {
        syntax_error("Expected an assign statement or expression. Found <STMNT %d>", stmnt->type);
    }
    return stmnt;
}

Stmnt* parse_stmnt_for() {
    expect_token('(');
    Stmnt** init = NULL;
    if (!is_token(';')) {
        buf_push(init, parse_for_init_update());
        while (match_token(',')) {
            buf_push(init, parse_for_init_update());
        }
    }
    expect_token(';');
    Expr* cond = NULL;
    if (!is_token(';')) {
        cond = parse_expr();
    }
    expect_token(';');
    Stmnt** update = NULL;
    if (!is_token(')')) {
        buf_push(update, parse_for_init_update());
        while (match_token(',')) {
            buf_push(update, parse_for_init_update());
        }
    }
    expect_token(')');
    return stmnt_for(buf_len(init), init, cond, buf_len(update), update, parse_blockstmnt());
}

Stmnt* parse_stmnt_assign() {
    Expr* left = parse_expr();
    TokenType op = token.type;
    expect_assign_op();
    return stmnt_assign(left, parse_expr(), op);
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