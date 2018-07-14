void* xmalloc(size_t size) {
    void* p = malloc(size);
    if (!p) {
        perror("xmalloc fail!");
        exit(1);
    }
    return p;
}

void* xcalloc(size_t nitems, size_t size) {
    void* p = calloc(nitems, size);
    if (!p) {
        perror("xcalloc fail!");
        exit(1);
    }
    return p;
}

void* xrealloc(void* block, size_t size) {
    void* p = realloc(block, size);
    if (!p) {
        perror("xmrealloc fail");
        exit(1);
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

void syntax_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("SYNTAX ERROR: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    getchar();
    exit(1);
}

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[0];
} BufHdr;

#define _buf_hdr(b) ((BufHdr*)((char*)(b) -  offsetof(BufHdr, buf)))
#define buf_len(b) ((b) ? _buf_hdr(b)->len : 0)
#define buf_cap(b) ((b) ? _buf_hdr(b)->cap : 0)

#define _buf_fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define _buf_fit(b, n) (_buf_fits(b, n) ? 0 : ((b) = _buf_grow((b), buf_len(b) + (n), sizeof(*(b)))))
#define buf_push(b, e) (_buf_fit(b, 1), (b)[buf_len(b)] = (e), _buf_hdr(b)->len++)
#define buf_free(b) ((b) ? (free(_buf_hdr(b)), (b) = NULL) : 0)

void* _buf_grow(const void* buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1) / 2);
    size_t new_cap = max(1 + 2 * buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr* new_hdr;
    if (buf) {
        new_hdr = realloc(_buf_hdr(buf), new_size);
    }
    else {
        new_hdr = malloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

typedef struct InternStr {
    size_t len;
    const char* str;
} InternStr;

static InternStr* interns;

const char* str_intern_range(const char* start, const char* end) {
    size_t len = end - start;
    for (size_t i = 0; i < buf_len(interns); i++)
        if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0)
            return interns[i].str;
    char* str = malloc(len + 1);
    memcpy(str, start, len);
    str[len] = 0;
    buf_push(interns, ((InternStr){len, str}));
    return str;
}

const char* str_intern(const char* str) {
    return str_intern_range(str, str + strlen(str));
}

void buf_test() {
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

intern_str_test() {
    char* a = "hi";
    char* b = "hi";
    assert(a == b);
    assert(str_intern(a) == str_intern(b));
    assert(str_intern(a) != str_intern("sdf"));
    printf("InternStr test passed\n");
}

void common_test() {
    buf_test();
    intern_str_test();
}
