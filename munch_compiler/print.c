void print_expr(Expr*);
void print_decl(Decl*);
void print_stmnt_block(BlockStmnt);

void print_str(const char* str) {
    printf("\"");
    for (size_t i = 0; i < strlen(str); i++) {
        if (esc_char_to_str[(size_t)str[i]]) {
            printf("%s", esc_char_to_str[(size_t)str[i]]);
        }
        else {
            printf("%c", str[i]);
        }
    }
    printf("\"");
}

void print_char(char c) {
    printf("'");
    (c != '"' && esc_char_to_str[(size_t)c]) ? printf("%s", esc_char_to_str[(size_t)c]) : (c == '\'' ? printf("\\'") : printf("%c", c));
    printf("'");
}

void print_op(TokenType op) {
    printf("%s", op_to_str(op));
}

int indent = 0;
#define ___ "                                              "

void print_new_line(void) {
    printf("\n%.*s", 2 * indent, ___ ___ ___ ___);
}

#undef ___

#define INDENT indent++; print_new_line()
#define UNINDENT indent--; print_new_line()

void print_stmnt(Stmnt* stmnt) {
    switch (stmnt->type) {
    case STMNT_DECL:
        print_decl(stmnt->decl_stmnt.decl);
        break;
    case STMNT_RETURN:
        printf("return ");
        if (stmnt->return_stmnt.expr) {
            print_expr(stmnt->return_stmnt.expr);
        }
        break;
    case STMNT_IF_ELSE:
        printf("if (");
        print_expr(stmnt->ifelseif_stmnt.if_cond);
        printf(") ");
        print_stmnt_block(stmnt->ifelseif_stmnt.then_block);
        for (size_t i = 0; i < stmnt->ifelseif_stmnt.num_else_ifs; i++) {
            printf(" else if (");
            print_expr(stmnt->ifelseif_stmnt.else_ifs[i].cond);
            printf(")");
            print_stmnt_block(stmnt->ifelseif_stmnt.else_ifs[i].block);
        }
        if (stmnt->ifelseif_stmnt.else_block.num_stmnts > 0) {
            printf(" else ");
            print_stmnt_block(stmnt->ifelseif_stmnt.else_block);
        }
        break;
    case STMNT_SWITCH:
        printf("switch (");
        print_expr(stmnt->switch_stmnt.switch_expr);
        printf(") {");
        INDENT;
        for (size_t i = 0; i < stmnt->switch_stmnt.num_case_blocks; i++) {
            printf("case (");
            print_expr(stmnt->switch_stmnt.case_blocks[i].case_expr);
            printf(")");
            print_stmnt_block(stmnt->switch_stmnt.case_blocks[i].block);
        }
        if (stmnt->switch_stmnt.default_block.num_stmnts > 0) {
            printf("default");
            print_stmnt_block(stmnt->switch_stmnt.default_block);
        }
        UNINDENT;
        printf("}");
        break;
    case STMNT_WHILE:
        printf("while (");
        print_expr(stmnt->while_stmnt.cond);
        printf(")");
        print_stmnt_block(stmnt->while_stmnt.block);
        break;
    case STMNT_DO_WHILE:
        printf("do");
        print_stmnt_block(stmnt->while_stmnt.block);
        printf(" while(");
        print_expr(stmnt->while_stmnt.cond);
        printf(")");
        break;
    case STMNT_FOR:
        printf("for (");
        for (size_t i = 0; i < stmnt->for_stmnt.num_init; i++) {
            print_stmnt(stmnt->for_stmnt.init[i]);
            if (i != stmnt->for_stmnt.num_init - 1) printf(", ");
        }
        printf("; ");
        print_expr(stmnt->for_stmnt.cond);
        printf("; ");
        for (size_t i = 0; i < stmnt->for_stmnt.num_update; i++) {
            print_stmnt(stmnt->for_stmnt.update[i]);
            if (i != stmnt->for_stmnt.num_update - 1) printf(", ");
        }
        printf(")");
        print_stmnt_block(stmnt->for_stmnt.block);
        break;
    case STMNT_ASSIGN:
        print_expr(stmnt->assign_stmnt.left);
        printf(" ");
        print_op(stmnt->assign_stmnt.op);
        printf(" ");
        print_expr(stmnt->assign_stmnt.right);
        break;
    case STMNT_INIT:
        print_expr(stmnt->init_stmnt.left);
        printf(" := ");
        print_expr(stmnt->init_stmnt.right);
        break;
    case STMNT_BREAK:
        printf("break");
        break;
    case STMNT_CONTINUE:
        printf("continue");
        break;
    case STMNT_BLOCK:
        print_stmnt_block(stmnt->block_stmnt);
        break;
    case STMNT_EXPR:
        print_expr(stmnt->expr_stmnt.expr);
        break;
    default:
        assert(0);
    }
}

void print_stmnt_block(BlockStmnt block) {
    printf("{ ");
    INDENT;
    for (size_t i = 0; i < block.num_stmnts; i++) {
        print_stmnt(block.stmnts[i]);
        if (i != block.num_stmnts - 1) print_new_line();
    }
    UNINDENT;
    printf("}");
}

void print_typespec(TypeSpec* typespec) {
    switch (typespec->type) {
    case TYPESPEC_NAME:
        printf("%s", typespec->name.name);
        break;
    case TYPESPEC_FUNC:
        printf("((");
        print_typespec(typespec->func.ret_type);
        printf(")(");
        for (size_t i = 0; i < typespec->func.num_params; i++) {
            print_typespec(typespec->func.params[i]);
            if(i != typespec->func.num_params - 1) printf(",");
        }
        printf("))");
        break;
    case TYPESPEC_ARRAY:
        printf("(");
        print_typespec(typespec->array.base);
        printf("[");
        print_expr(typespec->array.size);
        printf("])");
        break;
    case TYPESPEC_PTR:
        printf("(ref ");
        print_typespec(typespec->ptr.base);
        printf(")");
        break;
        break;
    default:
        assert(0);
    }
}

void print_expr(Expr* expr) {
    switch (expr->type) {
    case EXPR_TERNARY:
        printf("(");
        print_expr(expr->ternary_expr.cond);
        printf(")?(");
        print_expr(expr->ternary_expr.left);
        printf("):(");
        print_expr(expr->ternary_expr.right);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(");
        print_op(expr->binary_expr.op);
        printf(" ");
        print_expr(expr->binary_expr.left);
        printf(" ");
        print_expr(expr->binary_expr.right);
        printf(")");
        break;
    case EXPR_PRE_UNARY:
        print_op(expr->pre_unary_expr.op);
        print_expr(expr->pre_unary_expr.expr);
        break;
    case EXPR_POST_UNARY:
        print_expr(expr->pre_unary_expr.expr);
        print_op(expr->pre_unary_expr.op);
        break;
    case EXPR_CALL:
        printf("(call ");
        print_expr(expr->call_expr.expr);
        printf(" (");
        for (size_t i = 0; i < expr->call_expr.num_args; i++) {
            print_expr(expr->call_expr.args[i]);
            if(i != expr->call_expr.num_args - 1) printf(", ");
        }
        printf("))");
        break;
    case EXPR_INT:
        printf("%"PRId64, expr->int_expr.int_val);
        break;
    case EXPR_FLOAT:
        printf("%f", expr->float_expr.float_val);
        break;
    case EXPR_STR:
        print_str(expr->str_expr.str_val);
        break;
    case EXPR_NAME:
        printf("%s", expr->name_expr.name);
        break;
    case EXPR_COMPOUND:
        printf("(");
        if (expr->compound_expr.type) {
            printf("(");
            print_typespec(expr->compound_expr.type);
            printf(")");
        }
        printf("{");
        for (size_t i = 0; i < expr->compound_expr.num_compound_items; i++) {
            CompoundItem compound_item = expr->compound_expr.compound_items[i];
            switch (compound_item.type) {
            case COMPOUND_DEFAULT:
                break;
            case COMPOUND_INDEX:
                printf("[");
                print_expr(compound_item.index);
                printf("]");
                printf(" = ");
                break;
            case COMPOUND_NAME:
                printf("%s", compound_item.name);
                printf(" = ");
                break;
            default:
                assert(0);
            }
            print_expr(compound_item.value);
            if(i != expr->compound_expr.num_compound_items - 1) printf(", ");
        }
        printf("})");
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_typespec(expr->cast_expr.cast_type);
        printf(" ");
        print_expr(expr->cast_expr.cast_expr);
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(index ");
        print_expr(expr->index_expr.expr);
        printf(" [");
        print_expr(expr->index_expr.index);
        printf("]");
        break;
    case EXPR_FIELD:
        printf("(field ");
        print_expr(expr->field_expr.expr);
        printf(" %s)", expr->field_expr.field);
        break;
    case EXPR_SIZEOF_TYPE:
        printf("(sizeof :");
        print_typespec(expr->sizeof_expr.type);
        printf(")");
        break;
    case EXPR_SIZEOF_EXPR:
        printf("(sizeof ");
        print_expr(expr->sizeof_expr.expr);
        printf(")");
        break;
    default:
        assert(0);
    }
}

void print_decl(Decl* decl) {
    switch (decl->type) {
    case DECL_ENUM:
        printf("enum %s {", decl->name);
        INDENT;
        for (size_t i = 0; i < decl->enum_decl.num_enum_items; i++) {
            printf("%s", decl->enum_decl.enum_items[i].name);
            if (decl->enum_decl.enum_items[i].expr) {
                printf(" = ");
                print_expr(decl->enum_decl.enum_items[i].expr);
            }
            if (i != decl->enum_decl.num_enum_items - 1) {
                print_new_line();
            }
        }
        UNINDENT;
        printf("}");
        break;
    case DECL_STRUCT:
    case DECL_UNION:
        printf("%s %s {", decl->type == DECL_UNION ? "union" : "struct", decl->name);
        INDENT;
        for (size_t i = 0; i < decl->aggregate_decl.num_aggregate_items; i++) {
            AggregateItem aggr_item = decl->aggregate_decl.aggregate_items[i];
            print_typespec(aggr_item.type);
            printf(" %s", aggr_item.name);
            if (aggr_item.expr) {
                printf(" = ");
                print_expr(aggr_item.expr);
            }
            if (i != decl->aggregate_decl.num_aggregate_items - 1) {
                print_new_line();
            }
        }
        UNINDENT;
        printf(")");
        break;
    case DECL_CONST:
        printf("const %s := ", decl->name);
        print_expr(decl->const_decl.expr);
        break;
    case DECL_VAR:
        if (decl->var_decl.type) {
            print_typespec(decl->var_decl.type);
        }
        else {
            printf("var");
        }
        printf(" %s", decl->name);
        if (decl->var_decl.expr) {
            printf(" = ");
            print_expr(decl->var_decl.expr);
        }
        break;
    case DECL_TYPEDEF:
        printf("typedef %s = ", decl->name);
        print_typespec(decl->typedef_decl.type);
        break;
    case DECL_FUNC:
        printf("func %s", decl->name);
        if (decl->func_decl.ret_type) {
            printf("(");
            print_typespec(decl->func_decl.ret_type);
            printf(")");
        }
        printf("(");
        for (size_t i = 0; i < decl->func_decl.num_params; i++) {
            FuncParam param = decl->func_decl.params[i];
            print_typespec(param.type);
            printf(" %s", param.name);
            if (i != decl->func_decl.num_params - 1) printf(", ");
        }
        printf(") ");
        print_stmnt_block(decl->func_decl.block);
        break;
    default:
        assert(0);
    }
}

#undef INDENT
#undef UNINDENT

void print_expr_line(Expr* expr) {
    print_expr(expr);
    printf("\n");
}

void print_typespec_line(TypeSpec* typespec) {
    print_typespec(typespec);
    printf("\n");
}

void print_decl_line(Decl* decl) {
    print_decl(decl);
    printf("\n");
}

#define CREATE_PRINT_EXPR(e, create_expr) Expr* (e) = (create_expr); print_expr_line(e)
#define CREATE_PRINT_TYPE(t, create_type) TypeSpec* (t) = (create_type); print_typespec_line(t)
#define CREATE_PRINT_DECL(d, create_decl) Decl* (d) = (create_decl); print_decl_line(d)

void print_ast_test(void) {
    printf("----- print.c -----\n");

    CREATE_PRINT_TYPE(t1, typespec_name("list"));
    CREATE_PRINT_TYPE(t2, typespec_array(t1, expr_binary('+', expr_int(1), expr_int(2))));
    CREATE_PRINT_TYPE(t3, typespec_ptr(t2));
    CREATE_PRINT_TYPE(t4, typespec_func(t3, 2, (TypeSpec*[]) { typespec_name("int"), t2 }));
    
    CREATE_PRINT_EXPR(e1, expr_binary('+', expr_int(41), expr_int(2)));
    CREATE_PRINT_EXPR(e2, expr_binary(TOKEN_LSHIFT, expr_int(1), expr_int(221)));
    CREATE_PRINT_EXPR(e3, expr_binary('^', e1, e2));
    CREATE_PRINT_EXPR(e4, expr_unary(EXPR_PRE_UNARY, '!', e3));
    CREATE_PRINT_EXPR(e5, expr_ternary(e4, expr_call(expr_name("foo"), 2, (Expr*[]) { expr_int(2), expr_str("bar") }), expr_int(2)));
    CREATE_PRINT_EXPR(e6, expr_field(expr_cast(typespec_name("vector"),
                                               expr_compound(typespec_name("int"), 2, (CompoundItem[]) { 
                                                   { .type=COMPOUND_DEFAULT, .value=expr_str("a") }, { .type=COMPOUND_DEFAULT, .value=expr_str("b") }
                                                })), "size"));
    CREATE_PRINT_EXPR(e7, expr_index(expr_compound(t2, 1, (CompoundItem[]) { {.type=COMPOUND_DEFAULT, .value=expr_int(2)} }), expr_call(expr_name("foo"), 0, NULL)));

    CREATE_PRINT_DECL(d1, decl_enum("Animal", 2, (EnumItem[]) { { "dog", expr_int(0) }, { "cat", expr_int(1) } }));
    CREATE_PRINT_DECL(d2, decl_aggregate(DECL_STRUCT, "Student", 2, (AggregateItem[]) { { "name", typespec_name("string"), expr_str("NONE") }, { "age", typespec_name("int"), expr_int(0) } }));
    CREATE_PRINT_DECL(d3, decl_const("pi", expr_float(3.14)));
    CREATE_PRINT_DECL(d4, decl_var("a", t2, expr_name("NULL")));
    CREATE_PRINT_DECL(d5, decl_typedef("hell", t4));
    CREATE_PRINT_DECL(d6,
        decl_func("fibonacci",
            1,
            (FuncParam[]) {
                {"n", typespec_name("int") }
            },
            typespec_name("int"),
            (BlockStmnt) {
                2,
                (Stmnt*[]) {
                    stmnt_ifelseif(
                        expr_binary(
                            TOKEN_LTEQ,
                            expr_name("n"),
                            expr_int(1)
                        ),
                        (BlockStmnt) {
                            1,
                            (Stmnt*[]) {
                                stmnt_return(
                                    expr_name("n")
                                )
                            }
                        },
                        0,
                        NULL,
                        (BlockStmnt) {
                            0,
                            NULL
                        }
                    ),
                    stmnt_return(
                        expr_binary(
                            '+',
                            expr_call(
                                expr_name("fibonacci"),
                                1,
                                (Expr*[]) {
                                    expr_binary('-',
                                        expr_name("n"),
                                        expr_int(1)
                                    )
                                }
                            ),
                            expr_call(
                                expr_name("fibonacci"),
                                1,
                                (Expr*[]) {
                                    expr_binary('-',
                                        expr_name("n"),
                                        expr_int(2)
                                    )
                                }
                            )
                        )
                    )
                }
            }));
            
    printf("ast print test passed\n");
}

#undef CREATE_PRINT_EXPR
#undef CREATE_PRINT_TYPE
