Stmnt* parse_stmnt(void);
Expr* parse_expr(void);
Decl* parse_decl(void);
TypeSpec* parse_typespec(void);
Expr* parse_expr_cmp(void);
Stmnt* parse_stmnt_simple(void);

BlockStmnt parse_blockstmnt(void);

const char* parse_name(void) {
    const char* name = token.name;
    expect_token(TOKEN_NAME);
    return name;
}

TypeSpec* parse_type_func(void) {
    expect_token('(');
    TypeSpec** args = NULL;
    if (!is_token(')')) {
        buf_push(args, parse_typespec());
        while (match_token(',')) {
            buf_push(args, parse_typespec());
        }
    }
    expect_token(')');
    TypeSpec* ret_type = NULL;
    if (match_token(':')) {
        ret_type = parse_typespec();
    }
    return typespec_func(ret_type, buf_len(args), args);
}

TypeSpec* parse_type_base(void) {
    if (is_token(TOKEN_NAME)) {
        const char* name = token.name;
        next_token();
        return typespec_name(name);
    }
    else if (match_keyword(kwrd_func)) {
        return parse_type_func();
    }
    else if (match_token('(')) {
        TypeSpec* type = parse_typespec();
        expect_token(')');
        return type;
    }
    syntax_error("Expected a base type. Found %s", token_to_str(token));
    return NULL;
}

TypeSpec* parse_typespec(void) {
    TypeSpec* type = parse_type_base();
    while (true) {
        if (match_token('[')) {
            Expr* index = parse_expr();
            expect_token(']');
            type = typespec_array(type, index);
        }
        else if (match_token('*')) {
            type = typespec_ptr(type);
        }
        else {
            break;
        }
    }
    return type;
}

Expr* parse_expr_literal(void) {
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
    else if (match_keyword(kwrd_true)) {
        return expr_int(1);
    }
    else if (match_keyword(kwrd_false)) {
        return expr_int(0);
    }
    syntax_error("Expected a literal. Found %s", token_to_str(token));
    return NULL;
}

CompoundItem parse_compound_item() {
    if (match_token('[')) {
        Expr* index_expr = parse_expr();
        expect_token(']');
        expect_token('=');
        return compound_index(index_expr, parse_expr());
    }
    Expr* expr = parse_expr();
    if (expr->type == EXPR_NAME) {
        if (match_token('=')) {
            return compound_name(expr->name_expr.name, parse_expr());
        }
    }
    return compound_default(expr);
}

Expr* parse_expr_compound(TypeSpec* type) {
    expect_token('{');
    CompoundItem* compound_items = NULL;
    if (!is_token('}')) {
        buf_push(compound_items, parse_compound_item());
        while (match_token(',')) {
            if (is_token('}')) break;
            buf_push(compound_items, parse_compound_item());
        }
    }
    expect_token('}');
    return expr_compound(type, buf_len(compound_items), compound_items);
}

Expr* parse_expr_sizeof(void) {
    expect_token('(');
    Expr* expr = NULL;
    if (match_token(':')) {
        expr = expr_sizeof_type(parse_typespec());
    }
    else {
        expr = expr_sizeof_expr(parse_expr());
    }
    expect_token(')');
    return expr;
}

Expr* parse_expr_operand(void) {
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
        if (match_token(':')) {
            TypeSpec* type = parse_typespec();
            expect_token(')');
            return parse_expr_compound(type);
        }
        else {
            Expr* expr = parse_expr();
            expect_token(')');
            return expr;
        }
    }
    else if (match_keyword(kwrd_sizeof)) {
        return parse_expr_sizeof();
    }
    else if (match_keyword(kwrd_cast)) {
        expect_token('(');
        TypeSpec* type = parse_typespec();
        expect_token(',');
        Expr* expr = parse_expr();
        expect_token(')');
        return expr_cast(type, expr);
    }
    syntax_error("Expected an operand expression. Found %s", token_to_str(token));
    return NULL;
}

Expr* parse_expr_base(void) {
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
        else if (is_token(TOKEN_INC) || is_token(TOKEN_DEC)) {
            TokenType op = token.type;
            next_token();
            operand_expr = expr_unary(EXPR_POST_UNARY, op, operand_expr);
            break;
        }
        else {
            break;
        }
    }
    return operand_expr;
}

Expr* parse_expr_unary(void) {
    if (is_unary_op()) {
        TokenType unary_op = token.type;
        next_token();
        return expr_unary(EXPR_PRE_UNARY, unary_op, parse_expr_unary());
    }
    return parse_expr_base();
}

Expr* parse_expr_mul(void) {
    Expr* unary_expr = parse_expr_unary();
    while (is_mul_op()) {
        TokenType mul_op = token.type;
        next_token();
        unary_expr = expr_binary(mul_op, unary_expr, parse_expr_unary());
    }
    return unary_expr;
}

Expr* parse_expr_add(void) {
    Expr* mul_expr = parse_expr_mul();
    while (is_add_op()) {
        TokenType add_op = token.type;
        next_token();
        mul_expr = expr_binary(add_op, mul_expr, parse_expr_mul());
    }
    return mul_expr;
}

Expr* parse_expr_shift(void) {
    Expr* add_expr = parse_expr_add();
    if (is_shift_op()) {
        TokenType shift_op = token.type;
        next_token();
        add_expr = expr_binary(shift_op, add_expr, parse_expr_add());
    }
    return add_expr;
}

Expr* parse_expr_cmp(void) {
    Expr* shift_expr = parse_expr_shift();
    if (is_cmp_op()) {
        TokenType cmp_op = token.type;
        next_token();
        shift_expr = expr_binary(cmp_op, shift_expr, parse_expr_shift());
    }
    return shift_expr;
}

Expr* parse_expr_bit_xor(void) {
    Expr* cmp_expr = parse_expr_cmp();
    while (match_token('^')) {
        cmp_expr = expr_binary('^', cmp_expr, parse_expr_cmp());
    }
    return cmp_expr;
}

Expr* parse_expr_bit_and(void) {
    Expr* bit_xor_expr = parse_expr_bit_xor();
    while (match_token('&')) {
        bit_xor_expr = expr_binary('&', bit_xor_expr, parse_expr_bit_xor());
    }
    return bit_xor_expr;
}

Expr* parse_expr_bit_or(void) {
    Expr* bit_and_expr = parse_expr_bit_and();
    while (match_token('|')) {
        bit_and_expr = expr_binary('|', bit_and_expr, parse_expr_bit_and());
    }
    return bit_and_expr;
}

Expr* parse_expr_and(void) {
    Expr* bit_or_expr = parse_expr_bit_or();
    while (match_token(TOKEN_LOG_AND)) {
        bit_or_expr = expr_binary(TOKEN_LOG_AND, bit_or_expr, parse_expr_bit_or());
    }
    return bit_or_expr;
}

Expr* parse_expr_or(void) {
    Expr* and_expr = parse_expr_and();
    while (match_token(TOKEN_LOG_OR)) {
        and_expr = expr_binary(TOKEN_LOG_OR, and_expr, parse_expr_and());
    }
    return and_expr;
}

Expr* parse_expr_ternary(void) {
    Expr* or_expr = parse_expr_or();
    if (match_token('?')) {
        Expr* left = parse_expr_ternary();
        expect_token(':');
        Expr* right = parse_expr_ternary();
        return expr_ternary(or_expr, left, right);
    }
    return or_expr;
}

Expr* parse_expr(void) {
    return parse_expr_ternary();
}

Stmnt* parse_stmnt_ifelseif(void) {
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    BlockStmnt then_block = parse_blockstmnt();
    ElseIfItem* else_ifs = NULL;
    BlockStmnt else_block = (BlockStmnt) { 0, NULL };
    while (true) {
        if (match_keyword(kwrd_else)) {
            if (match_keyword(kwrd_if)) {
                expect_token('(');
                Expr* elseif_cond = parse_expr();
                expect_token(')');
                ElseIfItem elseif_item = (ElseIfItem) { elseif_cond, parse_blockstmnt() };
                buf_push(else_ifs, elseif_item);
            }
            else {
                else_block = parse_blockstmnt();
                break;
            }
        }
        else {
            break;
        }
    }
    return stmnt_ifelseif(cond, then_block, buf_len(else_ifs), else_ifs, else_block);
}

CaseBlock parse_case_block(void) {
    expect_keyword(kwrd_case);
    Expr* case_expr = parse_expr();
    expect_token(':');
    return (CaseBlock) { case_expr, parse_blockstmnt() };
}

Stmnt* parse_stmnt_switch(void) {
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

Stmnt* parse_stmnt_while(void) {
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    return stmnt_while(STMNT_WHILE, cond, parse_blockstmnt());
}

Stmnt* parse_stmnt_do(void) {
    BlockStmnt block = parse_blockstmnt();
    expect_keyword(kwrd_while);
    expect_token('(');
    Expr* cond = parse_expr();
    expect_token(')');
    expect_token(';');
    return stmnt_while(STMNT_DO_WHILE, cond, block);
}

Stmnt* parse_for_update(void) {
    Stmnt* stmnt = parse_stmnt_simple();
    if (stmnt->type != STMNT_ASSIGN && stmnt->type != STMNT_EXPR) {
        basic_syntax_error("Expected an assign statement or expression. Found <STMNT %d>", stmnt->type);
    }
    return stmnt;
}

Stmnt* parse_stmnt_for(void) {
    expect_token('(');
    Stmnt** init = NULL;
    if (!is_token(';')) {
        buf_push(init, parse_stmnt_simple());
        while (match_token(',')) {
            buf_push(init, parse_stmnt_simple());
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
        buf_push(update, parse_for_update());
        while (match_token(',')) {
            buf_push(update, parse_for_update());
        }
    }
    expect_token(')');
    return stmnt_for(buf_len(init), init, cond, buf_len(update), update, parse_blockstmnt());
}

Stmnt* parse_stmnt_assign(void) {
    Expr* left = parse_expr();
    TokenType op = token.type;
    if (match_token(TOKEN_COLON_ASSIGN)) {
        return stmnt_init(left, parse_expr());
    }
    expect_assign_op();
    return stmnt_assign(left, parse_expr(), op);
}

Stmnt* parse_stmnt_return(void) {
    Expr* expr = NULL;
    if (!is_token(';')) {
        expr = parse_expr();
    }
    expect_token(';');
    return stmnt_return(expr);
}

BlockStmnt parse_blockstmnt(void) {
    expect_token('{');
    Stmnt** stmnts = NULL;
    while (!is_token('}')) {
        buf_push(stmnts, parse_stmnt());
    }
    expect_token('}');
    return (BlockStmnt) { buf_len(stmnts), stmnts };
}

Stmnt* parse_stmnt_block(void) {
    expect_token('{');
    Stmnt** stmnts = NULL;
    while (!is_token('}')) {
        Stmnt* stmnt = parse_stmnt();
        if(stmnt) buf_push(stmnts, stmnt);
    }
    expect_token('}');
    return stmnt_block(buf_len(stmnts), stmnts);
}

Stmnt* parse_stmnt_expr(void) {
    return stmnt_expr(parse_expr());
}

Stmnt* parse_stmnt_simple(void) {
    if (is_token(';')) {
        return NULL;
    }
    Expr* left = parse_expr();
    if (is_assign_op()) {
        TokenType op = token.type;
        next_token();
        return stmnt_assign(left, parse_expr(), op);
    }
    else if (match_token(TOKEN_COLON_ASSIGN)) {
        return stmnt_init(left, parse_expr());
    }
    else {
        return stmnt_expr(left);
    }
}

Stmnt* parse_stmnt_decl(void) {
    Decl* decl = parse_decl();
    expect_token(';');
    return stmnt_decl(decl);
}

Stmnt* parse_stmnt(void) {
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
        expect_token(';');
        return stmnt_break();
    }
    else if (match_keyword(kwrd_continue)) {
        expect_token(';');
        return stmnt_continue();
    }
    else if (is_decl_keyword()) {
        return parse_stmnt_decl();
    } 
    else {
        Stmnt* stmnt = parse_stmnt_simple();
        expect_token(';');
        return stmnt;
    }
}

EnumItem parse_enum_item(void) {
    const char* name = parse_name();
    if (match_token('=')) {
        return (EnumItem) { name, parse_expr() };
    }
    return (EnumItem) { name, NULL };
}

Decl* parse_decl_enum(void) {
    const char* name = parse_name();
    expect_token('{');
    EnumItem* enum_items = NULL;
    buf_push(enum_items, parse_enum_item());
    while (match_token(',')) {
        if (is_token('}')) break;
        buf_push(enum_items, parse_enum_item());
    }
    expect_token('}');
    return decl_enum(name, buf_len(enum_items), enum_items);
}

AggregateItem parse_aggregate_item(void) {
    const char* name = parse_name();
    if (match_token(':')) {
        TypeSpec* type = parse_typespec();
        if (match_token('=')) {
            return (AggregateItem) { name, type, parse_expr() };
        }
        return (AggregateItem) { name, type, NULL };
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
    expect_token(';');
    while (!is_token('}')) {
        buf_push(aggregate_items, parse_aggregate_item());
        expect_token(';');
    }
    expect_token('}');
    return decl_aggregate(type, name, buf_len(aggregate_items), aggregate_items);
}

Decl* parse_decl_struct(void) {
    return parse_aggregate(DECL_STRUCT);
}

Decl* parse_decl_union(void) {
    return parse_aggregate(DECL_UNION);
}

Decl* parse_decl_const(void) {
    const char* name = parse_name();
    expect_token('=');
    return decl_const(name, parse_expr());
}

Decl* parse_decl_var(void) {
    const char* name = parse_name();
    if (match_token(':')) {
        TypeSpec* type = parse_typespec();
        if (match_token('=')) {
            return decl_var(name, type, parse_expr());
        }
        return decl_var(name, type, default_expr_compound());
    }
    else if (match_token('=')) {
        return decl_var(name, NULL, parse_expr());
    }
    else {
        syntax_error("Expected = or : in var declaration. Found %s", tokentype_to_str(token.type));
        return NULL;
    }
}

Decl* parse_decl_typedef(void) {
    const char* name = parse_name();
    expect_token('=');
    return decl_typedef(name, parse_typespec());
}

FuncParam parse_func_param(void) {
    const char* name = parse_name();
    expect_token(':');
    TypeSpec* type = parse_typespec();
    return (FuncParam) { name, type };
}

Decl* parse_decl_func(void) {
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
    if (match_token(':')) {
        ret_type = parse_typespec();
    }
    else {
        ret_type = typespec_name(str_intern("void"));
    }
    return decl_func(name, buf_len(func_params), func_params, ret_type, parse_blockstmnt());
}

Decl* parse_decl(void) {
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
    syntax_error("Expected a declaration keyword. Found %s", token_to_str(token));
    return NULL;
}

DeclSet* parse_stream(void) {
    Decl** decls = NULL;
    while (token.type != TOKEN_EOF) {
        buf_push(decls, parse_decl());
    }
    return declset(decls, buf_len(decls));
}

#define PARSE_SRC_DECL(src) init_stream(src); print_decl(parse_decl()); printf("\n-----------------------------\n")
#define PARSE_SRC_STMNT(src) init_stream(src); print_stmnt(parse_stmnt()); printf("\n-----------------------------\n")

void parse_test(void) {
    printf("----- parse.c -----\n");
    
    init_keywords();

    PARSE_SRC_STMNT("a > b ? ++a: b;");
    PARSE_SRC_STMNT("x + y++ * z;");
    PARSE_SRC_STMNT("x + --y++ * z;");
    PARSE_SRC_STMNT("size += {1, 2, 3}.length;");
    PARSE_SRC_STMNT("size += (:int[3]) {1, 2, 3}.length;");
    PARSE_SRC_STMNT("if(a == 1 || b <= a++ / 2) { b++; } else if (c > 2 ? b : a) { d[i][j].mm.s(2, 2); } else if (i <= 3 << 2) {d++;} else {a++; if(i < 2) {d /= gcd(a, b);}}");
    PARSE_SRC_STMNT("for(i = 0; i < 10; i++, ++k) {printf(\"hello %d\", i); if(false) {continue;}}");
    PARSE_SRC_STMNT("{while(i < 10 && **j == 20) {a[2][b[3]]=i;} a++;}");
    PARSE_SRC_STMNT("do{ forever(\"Hi \\\"Isura\\\"\");} while(true);");
    PARSE_SRC_STMNT("{ ;;a++;;;;;}");
    PARSE_SRC_STMNT("{ a++; a = sizeof(b + c) / sizeof(:int[3]); }");

    PARSE_SRC_DECL("var v : int = 1");
    PARSE_SRC_DECL("enum Animal {dog, cat=2 + 1,}");
    PARSE_SRC_DECL("struct Student {name :String; age: int; classes: Class[10]; id:int = -1; class: Class**;}");
    PARSE_SRC_DECL("typedef foo = func (int, int**):String[10]");
    PARSE_SRC_DECL("func fibonacci(n: int): int {if(n <= 1) { return n; } return fibonacci(n-1) + fibonacci(n-2);}");
    PARSE_SRC_DECL("func kick_animal(a: Animal) {"
                        "switch(a) { "
                            "case Animal.dog:{"
                                "printf(\"woof\");"
                                "break;"
                            "}"
                            "case Animal.cat: {"
                                "printf(\"meow\");"
                                "break;"
                            "}"
                            "default: {"
                                "printf(\"hello there\");"
                            "}"
                        "}"
                    "}");
    PARSE_SRC_DECL("var v = Animal{name = \"dog\"}");
    PARSE_SRC_DECL("var w = (:int[1 << 8]){1, 3, ['a'] = 3, [32] = 11}");
    PARSE_SRC_DECL("func f(a: Animal): int { var a = b; return 2;}");
    PARSE_SRC_DECL("func add(a: V, b: V): V { var c: V; c = {a.x + b.x, a.y + b.y}; return c; }");
    PARSE_SRC_DECL("func f() {for(i := 0, j := 0, k = 0; i < 10; i++, k := 0) { print(i); } }");

    const char* src = "enum Animal {dog, cat=2 + 1,}\n"
        "struct Student {name :String; age: int; classes: Class[10]; id:int = -1; class: Class**;}";

    init_stream(src);
    DeclSet* declset = parse_stream();
    for (size_t i = 0; i < declset->num_decls; i++) {
        print_decl(declset->decls[i]);
        printf("\n");
    }

    printf("parse test passed\n");
}

#undef PARSE_SRC_DECL
#undef PARSE_SRC_STMNT
