#define TEST(test_method) test_method; printf("\n") 

void run_tests(void) {
    init_keywords();
    init_rand64();
    //TEST(rand_test());
    TEST(common_test());
    //TEST(lex_test());
    //TEST(print_ast_test());
    //TEST(parse_test());
    //TEST(resolve_test());
    //TEST(gen_test());
    //TEST(munch_test());
}

#undef TEST