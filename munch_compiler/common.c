void* xmalloc(size_t size) {
    void* p = malloc(size);
    if (!p) {
        perror("xmalloc fail!");
        getchar();
        exit(2001);
    }
    return p;
}

void* xcalloc(size_t nitems, size_t size) {
    void* p = calloc(nitems, size);
    if (!p) {
        perror("xcalloc fail!");
        getchar();
        exit(2002);
    }
    return p;
}

void* xrealloc(void* block, size_t size) {
    void* p = realloc(block, size);
    if (!p) {
        perror("xrealloc fail");
        getchar();
        exit(2003);
    }
    return p;
}

void fatal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    getchar();
    exit(1);
}

void basic_syntax_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("SYNTAX ERROR: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    getchar();
    exit(1);
}

#define is_between(x, a, b) ((x) >= (a) && (x) <= (b))

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[0];
} BufHdr;

#define _buf_hdr(b) ((BufHdr*)((char*)(b) -  offsetof(BufHdr, buf)))
#define buf_len(b) ((b) ? _buf_hdr(b)->len : 0)
#define buf_cap(b) ((b) ? _buf_hdr(b)->cap : 0)
#define buf_size(b) (buf_len(b) * sizeof(*(b)))

#define _buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define _buf_fit(b, n) (_buf_fits(b, n) ? 0 : ((b) = _buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))
#define buf_push(b, e) (_buf_fit(b, 1), (b)[buf_len(b)] = (e), _buf_hdr(b)->len++)
#define buf_end(b) (b + buf_len(b))
#define buf_free(b) ((b) ? (free(_buf_hdr(b)), (b) = NULL) : 0)

void* _buf_grow(const void* buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1) / 2);
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr* new_hdr;
    if (buf) {
        new_hdr = xrealloc(_buf_hdr(buf), new_size);
    }
    else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

#define _STR(x) #x
#define STR(x) _STR(x)
#define TODO(x) message(":warning:TODO: " #x)

#define ARENA_ALIGNMENT 8 // must be power of 2
#define ARENA_BLOCK_SIZE (1 << 10)

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, a)
#define ALIGN_DOWN_PTR(ptr, a) (void*) ALIGN_DOWN((uintptr_t)ptr, a)
#define ALIGN_UP_PTR(ptr, a) (void*) ALIGN_UP((uintptr_t)ptr, a)

typedef struct Arena {
    char* ptr;
    char* end;
    char** blocks;
} Arena;

void arena_grow(Arena* arena, size_t size) {
    size_t alloc_size = ALIGN_UP(max(size, ARENA_BLOCK_SIZE), ARENA_ALIGNMENT);
    arena->ptr = xmalloc(alloc_size);
    arena->end = arena->ptr + alloc_size;
    buf_push(arena->blocks, arena->ptr);
}

void* arena_alloc(Arena* arena, size_t size) {
    if (size > (size_t)(arena->end - arena->ptr)) {
        arena_grow(arena, size);
    }
    void* new_ptr = arena->ptr;
    arena->ptr = ALIGN_UP_PTR(new_ptr + size, ARENA_ALIGNMENT);
    assert(arena->end - arena->ptr >= 0);
    assert(new_ptr == ALIGN_DOWN_PTR(new_ptr, ARENA_ALIGNMENT));
    return new_ptr;
}

void arena_free(Arena* arena) {
    for (char** it = arena->blocks; it != buf_end(arena->blocks); it++) {
        free(*it);
    }
    buf_free(arena->blocks);
}

#undef ALIGN_DOWN
#undef ALIGN_UP
#undef ALIGN_PTR_UP
#undef ALIGN_PTR_DOWN

typedef struct InternStr {
    size_t len;
    const char* str;
} InternStr;

InternStr* interns;
Arena str_arena;

const char* str_intern_range(const char* start, const char* end) {
    size_t len = end - start;
    for (size_t i = 0; i < buf_len(interns); i++)
        if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0)
            return interns[i].str;
    char* str = arena_alloc(&str_arena, len + 1);
    memcpy(str, start, len);
    str[len] = 0;
    buf_push(interns, ((InternStr){len, str}));
    return str;
}

const char* str_intern(const char* str) {
    return str_intern_range(str, str + strlen(str));
}

void buf_test(void) {
    int* buf = NULL;
    assert(buf_len(buf) == 0);
    assert(buf_cap(buf) == 0);
    for (int i = 0; i < 10; i++) {
        buf_push(buf, i);
    }
    assert(buf_len(buf) == 10);
    assert(buf_cap(buf) >= 10);
    for (int i = 0; i < 10; i++) {
        assert(buf[i] == i);
    }
    buf_free(buf);
    assert(buf_len(buf) == 0);
    printf("BufHdr test passed\n");
}

void intern_str_test(void) {
    char* a = "hi";
    char* b = "hi";
    assert(a == b);
    assert(str_intern(a) == str_intern(b));
    assert(str_intern(a) != str_intern("sdf"));
    printf("InternStr test passed\n");
}

void common_test(void) {
    printf("----- common.c -----\n");
    buf_test();
    intern_str_test();
}
