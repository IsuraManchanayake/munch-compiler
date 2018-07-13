#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

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

void* xmalloc(size_t size) {
    void* p = malloc(size);
    if (!p) {
        perror("xmalloc fail!");
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

const char* kwrd_if;
const char* kwrd_else;
const char* kwrd_for;
const char* kwrd_while;
const char* kwrd_do;

void init_keywords() {
    kwrd_if = str_intern("if");
    kwrd_else = str_intern("else");
    kwrd_for = str_intern("for");
    kwrd_while = str_intern("while");
    kwrd_do = str_intern("do");
}

typedef enum TokenType {
    TOKEN_LAST_CHAR = 127,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STR,
    TOKEN_NAME
} TokenType;

typedef enum TokenMod {
    TOK_MOD_NONE,
    TOK_MOD_CHAR,
    TOK_MOD_BIN,
    TOK_MOD_OCT,
    TOK_MOD_HEX
} TokenMod;

typedef struct Token {
    TokenType type;
    TokenMod mod;
    const char* start;
    const char* end;
    union {
        uint64_t intval;
        double floatval;
        const char* strval;
        const char* name;
    };
} Token;

Token token;
const char* stream;

const char char_to_digit[256] = {
    ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
    ['a'] = 10, ['A'] = 10, ['b'] = 11, ['B'] = 11, ['c'] = 12, ['C'] = 12,
    ['d'] = 13, ['D'] = 13, ['e'] = 14, ['E'] = 14, ['f'] = 15, ['F'] = 15
};

void scan_int() {
    uint64_t val = 0;
    uint64_t base = 10;
    if (*stream == '0') {
        stream++;
        if (tolower(*stream) == 'x') {
            token.mod = TOK_MOD_HEX;
            base = 16;
            stream++;
        }
        else if (tolower(*stream) == 'b') {
            token.mod = TOK_MOD_BIN;
            base = 2;
            stream++;
        }
        else if (isdigit(*stream)) {
            token.mod = TOK_MOD_OCT;
            base = 8;
        }
        else {
            syntax_error("Invalid integer base prefix %c", *stream);
            stream++;
        }
    }
    for(;;) {
        uint64_t digit = char_to_digit[*stream];
        if (digit == 0 && *stream != '0') {
            break;
        }
        if (digit >= base) {
            syntax_error("Invalid integer literal %d for base %d", digit, base);
            digit = 0;
        }
        if (val > (UINT64_MAX - digit) / base) {
            syntax_error("Integer literal overflow");
            while (isdigit(*stream)) stream++;
            val = 0;
        }
        val = val * base + digit;
        stream++;
    }
    token.type = TOKEN_INT;
    token.intval = val;
}

void scan_float() {
    const char* start = stream;
    while (isdigit(*stream)) stream++;
    stream++;
    while (isdigit(*stream)) stream++;
    if (tolower(*stream) == 'e') {
        stream++;
        if (*stream != '+' && *stream != '-' && !isdigit(*stream)) {
            syntax_error("Expected digit or sign after exponent in the float literal. Found '%c'", *stream);
        }
        if (!isdigit(*stream)) stream++;
        while (isdigit(*stream)) stream++;
    }
    const char* end = stream;
    if (end - start == 1) {
        syntax_error("Float literal length should be more than 1");
    }
    double val = strtod(start, NULL);
    if (val == HUGE_VAL || val == -HUGE_VAL) {
        syntax_error("Float literal overflow");
    }
    token.type = TOKEN_FLOAT;
    token.floatval = val;
}

const char* escape_to_char[256] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['f'] = '\f',
    ['0'] = '\0',
};

void scan_char() {
    stream++;
    char val;
    if (*stream == '\n' || *stream == '\r' || *stream == '\a' || 
        *stream == '\b' || *stream == '\f' || *stream == '\v') {
        syntax_error("Char literals cannot have escape characters inside quotes. Found ASCII %d", (int)(*stream));
    }
    if (*stream == '\\') {
        stream++;
        val = escape_to_char[*stream];
        if (val == 0 && *stream != '0') {
            syntax_error("Undefined escape char literal");
        }
        stream++;
    }
    else {
        val = *stream;
        stream++;
    }
    if(*stream != '\'') {
        syntax_error("Char literal should be ended with '\''. Found '%c'", *stream);
    }
    stream++;
    token.type = TOKEN_INT;
    token.mod = TOK_MOD_CHAR;
    token.intval = val;
}

void scan_str() {
    stream++;
    char* str_buf = NULL;
    while (*stream && *stream != '"') {
        char val;
        if (*stream == '\a' || *stream == '\b' || *stream == '\f' || *stream == '\v') {
            syntax_error("String literals cannot have escape characters inside quotes. Found ASCII %d", (int)(*stream));
        }
        if (*stream == '\n' || *stream == '\r') {
            buf_push(str_buf, *stream);
            stream++;
        }
        if (*stream == '\\') {
            stream++;
            val = escape_to_char[*stream];
            if (val == 0 && *stream != '0') {
                syntax_error("Undefined escape char literal");
            }
            buf_push(str_buf, val);
            stream++;
        }
        else {
            val = *stream;
            buf_push(str_buf, val);
            stream++;
        }
    }
    if (*stream != '"') {
        syntax_error("String literal should be ended with '\"'. Found %c", *stream);
    }
    buf_push(str_buf, 0);
    stream++;
    token.type = TOKEN_STR;
    token.strval = str_buf;
}

void next_token() {
    token.start = stream;
    switch (*stream) {
    case ' ': case '\n': case '\t': case '\r': case '\v': case '\b': case '\f': case '\a': {
        while (isspace(*stream)) stream++;
        next_token();
        break;
    }
    case '.': {
        scan_float();
        break;
    }
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        const char* start = stream;
        while (isdigit(*stream)) stream++;
        if (*stream == '.' || *stream == 'e') {
            stream = start;
            scan_float();
        }
        else {
            stream = start;
            scan_int();
        }
        break;
    }
    case '\'': {
        scan_char();
        break;
    }
    case '"': {
        scan_str();
        break;
    }
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': 
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': 
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': 
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        char* start = stream++;
        while (isalnum(*stream) || *stream == '_') stream++;
        token.type = TOKEN_NAME;
        token.name = str_intern_range(start, stream);
        break;
    }
    default:
        token.type = *stream++;
    }
    token.end = stream;
}

void init_stream(const char* src) {
    stream = src;
    next_token();
}

inline bool is_token(TokenType type) {
    return token.type == type;
}

inline bool is_token_name(const char* name) {
    return token.type == TOKEN_NAME && token.name == name;
}

inline bool match_token(TokenType type) {
    if (is_token(type)) {
        next_token();
        return true;
    }
    else {
        return false;
    }
}

const char* get_token_type_name(TokenType type) {
    static char buf[256];
    switch (type) {
    case TOKEN_LAST_CHAR:
        sprintf_s(buf, 256, "TOKEN_LAST_CHAR");
        break;
    case TOKEN_INT:
        sprintf_s(buf, 256, "TOKEN_INT");
        break;
    case TOKEN_FLOAT:
        sprintf_s(buf, 256, "TOKEN_FLOAT");
        break;
    case TOKEN_STR:
        sprintf_s(buf, 256, "TOKEN_STR");
        break;
    case TOKEN_NAME:
        sprintf_s(buf, 256, "TOKEN_NAME");
        break;
    default:
        if (type < 128 && isprint(type)) {
            sprintf_s(buf, 256, "%c", type);
        }
        else {
            sprintf_s(buf, 256, "<ASCII %d>", type);
        }
    }
    return buf;
}

inline bool expect_token(TokenType type) {
    if (is_token(type)) {
        next_token();
        return true;
    }
    else {
        fatal("expected token %s. got %s", get_token_type_name(type), get_token_type_name(token.type));
        return false;
    }
}

int8_t* op_seq;

typedef enum Ops {
    LIT,
    ADD,
    SUB,
    MUL,
    DIV,
    NEG,
    HALT
};

/*
e3 = INT | '(' e ')'
e2 = [-] e3
e1 = e2 ([/*] e2)*
e0 = e1 ([+-] e1)*
e = e0
*/
int32_t parse_e();

int32_t parse_e3() {
    if (is_token(TOKEN_INT)) {
        int32_t val = token.intval;
        next_token();
        buf_push(op_seq, LIT);
        buf_push(op_seq, val);
        return val;
    }
    else if (is_token('(')) {
        next_token();
        int32_t val = parse_e();
        expect_token(')');
        return val;
    }
    else {
        fatal("expected int or expr. got %s", get_token_type_name(token.type));
    }
}

int32_t parse_e2() {
    if (is_token('-')) {
        next_token();
        int32_t val = parse_e2();
        buf_push(op_seq, NEG);
        return -val;
    }
    else {
        return parse_e3();
    }
}

int32_t parse_e1() {
    int32_t val = parse_e2();
    while (is_token('/') || is_token('*')) {
        char op = token.type;
        next_token();
        int32_t res = parse_e2();
        if (op == '*') {
            val *= res;
            buf_push(op_seq, MUL);
        }
        else {
            assert(res != 0);
            val /= res;
            buf_push(op_seq, DIV);
        }
    }
    return val;
}

int32_t parse_e0() {
    int32_t val = parse_e1();
    while (is_token('+') || is_token('-')) {
        char op = token.type;
        next_token();
        int32_t res = parse_e1();
        if (op == '+') {
            val += res;
            buf_push(op_seq, ADD);
        }
        else {
            val -= res;
            buf_push(op_seq, SUB);
        }
    }
    return val;
}

int32_t parse_e() {
    buf_free(op_seq);
    int32_t res = parse_e0();
    buf_push(op_seq, HALT);
    return res;
}

// vm -------------
#define MAX_STACK (1 << 10)

#define POPS(n) (assert(top >= (n) + stack))
#define POP() (*--top)
#define PUSHES(n) (assert(top + (n) <= stack + MAX_STACK))
#define PUSH(x) (*top++ = (x))

void vm_exec(const uint8_t* code) {
    int32_t stack[MAX_STACK];
    int32_t* top = stack;
    for (;;) {
        uint8_t op = *code++;
        printf("OP: %d\n", op);
        switch (op) {
        case ADD: {
            POPS(2);
            int32_t rv = POP();
            int32_t lv = POP();
            PUSHES(1);
            PUSH(lv + rv);
            printf("pushed: %d\n", lv + rv);
            break;
        }
        case SUB: {
            POPS(2);
            int32_t rv = POP();
            int32_t lv = POP();
            PUSHES(1);
            PUSH(lv - rv);
            break;
        }
        case MUL: {
            POPS(2);
            int32_t rv = POP();
            int32_t lv = POP();
            PUSHES(1);
            PUSH(lv * rv);
            break;
        }
        case DIV: {
            POPS(2);
            int32_t rv = POP();
            int32_t lv = POP();
            PUSHES(1);
            assert(rv != 0);
            PUSH(lv / rv);
            break;
        }
        case NEG: {
            POPS(1);
            int32_t v = POP();
            PUSHES(1);
            PUSH(-v);
            break;
        }
        case LIT: {
            PUSHES(1);
            PUSH(*(int32_t*)code);
            code += sizeof(int32_t);
            break;
        }
        case HALT:
            POPS(1);
            printf("vm_result: %lld", POP());
            return;
        default:
            fatal("vm_exec: unidentified op %d", op);
        }
    }
}
// vm end

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
    assert(buf_len(buf) == NULL);
}

void print_token(Token token) {
    switch (token.type) {
    case TOKEN_INT:
        printf("TOKEN: TOKEN_INT %llu\n", token.intval);
        break;
    case TOKEN_FLOAT:
        printf("TOKEN: TOKEN_FLOAT %f\n", token.floatval);
        break;
    case TOKEN_STR:
        printf("TOKEN: TOKEN_STR %s\n", token.strval);
        break;
    case TOKEN_NAME:
        printf("TOKEN: TOKEN_NAME %s %0X\n", token.name, (int)token.name);
        break;
    default:
        printf("TOKEN: %c\n", token.type);
    }
}

#define assert_token_type(type) (assert(match_token((type))))
#define assert_token_name(x) (assert(str_intern((x)) == token.name && match_token(TOKEN_NAME)))
#define assert_token_float(x) (assert(token.floatval == (x) && match_token(TOKEN_FLOAT)))
#define assert_token_int(x) (assert(token.intval == (x) && match_token(TOKEN_INT)))
#define assert_token_str(x) (assert(strcmp(token.strval, (x)) == 0 && match_token(TOKEN_STR)))
#define assert_token_char(x) (assert(token.intval == (x) && token.mod == TOK_MOD_CHAR && match_token(TOKEN_INT)))
#define assert_token_eof() (assert(is_token(0)))
#define init_stream(src) (stream = (src), next_token())

lex_test() {
    init_stream("032  + 0x111f");
    assert_token_int(032);
    assert_token_type('+');
    assert_token_int(0x111f);
    assert_token_eof();

    init_stream("4.123E32 0.32 .323 .232e-2 1e34");
    assert_token_float(4.123E32);
    assert_token_float(0.32);
    assert_token_float(.323);
    assert_token_float(.232e-2);
    assert_token_float(1e34);
    assert_token_eof();

    init_stream("'a' '\\n'  '\\a''\\0'");
    assert_token_char('a');
    assert_token_char('\n');
    assert_token_char('\a');
    assert_token_char('\0');
    assert_token_eof();

    init_stream("\"sdfasf\" \"hel\nlo\\n world!\"");
    assert_token_str("sdfasf");
    assert_token_str("hel\nlo\n world!");
    assert_token_eof();
}

#undef assert_token_type
#undef assert_token_int
#undef assert_token_name
#undef assert_token_eof

intern_str_test() {
    char* a = "hi";
    char* b = "hi";
    assert(a == b);
    assert(str_intern(a) == str_intern(b));
    assert(str_intern(a) != str_intern("sdf"));
}

int32_t parse_src(const char* src) {
    init_stream(src);
    //printf("Evaluating \"%s\"\n", src);
    int32_t res = parse_e();
    //printf("Result     %d\n\n", res);
    return res;
}

#define TEST_PARSE(expr) (assert(parse_src(#expr), (expr)))
parse_test() {
    TEST_PARSE(10 + 1);
    TEST_PARSE(2);
    TEST_PARSE(-23423423);
    TEST_PARSE(-(3 -5));
    TEST_PARSE(4 / 2+1);
    TEST_PARSE(-1*(2+(3-6/3)));
}
#undef TEST_PARSE

vm_test() {
    parse_src("1+2+3");
    buf_push(op_seq, HALT);
    vm_exec(op_seq);
}

test() {
    buf_test();
    lex_test();
    intern_str_test();
    parse_test();
    //vm_test();
}

int main(int argc, char** argv) {
    test();
    getchar();
    return 0;
}
