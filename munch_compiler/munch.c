void munch_init(const char* src) {
    init_keywords();
    init_stream(src);
    install_built_in_types();
    install_built_in_consts();
}

const char* munch_compile_str(const char* src) {
    munch_init(src);
    DeclSet* declset = parse_stream();
    install_decls(declset);
    complete_entities();
    gen_all();
    return gen_buf;
}

bool munch_compile_file(const char* path) {
    char* src = read_file(path);
    if (!src) {
        src = " ";
    }
    const char* buf = munch_compile_str(src);
    if (!buf) {
        return false;
    }
    char* out_path = change_ext(path, "c");
    return write_file(out_path, buf, buf_len(buf));
}

int munch_main(int argc, char** argv) {
    if(argc < 2) {
        printf("Usage: <source file>");
        return 1;
    }
    char* path = argv[1];
    bool status = munch_compile_file(path);
    if(status) {
        puts("Compilation successful\n");
    } else {
        puts("Compilation failed\n");
    }
    return status;
}

void munch_test(void) {
    if (munch_compile_file("munch_test/test1.mch")) {
        puts("Compilation successful\n");
    }
    else {
        puts("Compilation failed\n");
    }
}
