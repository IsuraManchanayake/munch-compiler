#define TEST(test_method) test_method; printf("\n") 

void run_tests() {
    init_keywords();
    //TEST(common_test());
    //TEST(lex_test());
    //TEST(print_ast_test());
    //TEST(parse_test());
    TEST(resolve_test());
}

#undef TEST
