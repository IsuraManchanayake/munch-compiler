#pragma TODO("PARSE ALL!!!")

parse_decl() {

}

parse_test() {
    init_stream("const pi = 3.14");
    parse_decl();

    printf("parse test passed\n");
}