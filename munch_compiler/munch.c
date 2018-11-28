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
    src_path = path;
    const char* buf = munch_compile_str(src);
    if (!buf) {
        return false;
    }
    char* out_path = change_ext(path, "c");
    return write_file(out_path, buf, buf_len(buf));
}

const char* arg_src_path;

void parse_args(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Usage: <source file> [-W-no]");
        exit(1);
    }
    if (argc == 3 && strcmp(argv[2], "-W-no") == 0) {
        enable_warnings = false;
    }
    arg_src_path = argv[1];
}

int munch_main(int argc, char** argv) {
    parse_args(argc, argv);
    bool status = munch_compile_file(arg_src_path);
    if (status) {
        puts("Compilation successful\n");
    }
    else {
        puts("Compilation failed\n");
    }
    printf("Collisions: %zu\n", collisions);
    printf("Map collisions: %zu\n", map_collisions);
    printf("Max probing   : %zu\n", max_probing);
    printf("Avg probing   : %.2f\n", (map_collisions + 0.0f) / map_put_n);
    printf("Intern map len: %zu\n", intern_map.len);
    printf("Intern map cap: %zu\n", intern_map.cap);
    printf("Intern arena size: %zu\n", ARENA_BLOCK_SIZE * buf_len(str_arena.blocks));
    return status;
}

void munch_test(void) {
    if (munch_compile_file("munch_test/test1024.mch")) {
        puts("Compilation successful\n");
    }
    else {
        puts("Compilation failed\n");
    }
}
