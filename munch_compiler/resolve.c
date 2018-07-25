typedef enum SymState {
    SYM_STATE_UNRESOLVED,
    SYM_STATE_RESOLVING,
    SYM_STATE_RESOLVED
} SymState;

typedef struct Sym {
    const char* name;
    SymState state;
    Decl* decl;
} Sym;

Sym** syms;

// name should be intern str
Sym* get_sym(const char* name) {
    for (size_t i = 0; i < buf_len(syms); i++) {
        if (name == syms[i]->name) {
            return syms[i];
        }
    }
    return NULL;
}

resolve_test() {
    printf("resolve test passed");
}