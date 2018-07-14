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
    TOKEN_NAME,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_LOG_AND,
    TOKEN_LOG_OR,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_COLON_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_BIT_AND_ASSIGN,
    TOKEN_BIT_OR_ASSIGN,
    TOKEN_BIT_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN
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
        else { // just 0
            token.type = TOKEN_INT;
            token.intval = 0;
            return;
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

const char escape_to_char[256] = {
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
#define CASE_1(op_start, op_1, tok_1) \
    case op_start: \
        token.type = op_start; \
        stream++; \
        if (*stream == op_1) { \
            token.type = tok_1; \
            stream++; \
        } \
        break;
#define CASE_2(op_start, op_1, tok_1, op_2, tok_2) \
    case op_start: \
        token.type = op_start; \
        stream++; \
        if (*stream == op_1) { \
            token.type = tok_1; \
            stream++; \
        } \
        else if (*stream == op_2) { \
            token.type = tok_2; \
            stream++; \
        } \
        break;
#define CASE_LONG(op_start, ch_1, tok_1, ch_2, tok_2, ch_3, tok_3) \
    case op_start: \
        token.type = op_start; \
        stream++; \
        if (*stream == ch_1) { \
            token.type = tok_1; \
            stream++; \
            if (*stream == ch_2) { \
                token.type = tok_2; \
                stream++; \
            } \
        } else if(*stream == ch_3) { \
            token.type = tok_3; \
            stream++; \
        } \
        break;
    CASE_1('=', '=', TOKEN_EQ);
    CASE_1('!', '=', TOKEN_NEQ);
    CASE_1(':', '=', TOKEN_COLON_ASSIGN);
    CASE_1('/', '=', TOKEN_DIV_ASSIGN);
    CASE_1('*', '=', TOKEN_MUL_ASSIGN);
    CASE_1('%', '=', TOKEN_MOD_ASSIGN);
    CASE_1('^', '=', TOKEN_BIT_XOR_ASSIGN);
    CASE_2('+', '+', TOKEN_INC, '=', TOKEN_ADD_ASSIGN);
    CASE_2('-', '-', TOKEN_DEC, '=', TOKEN_SUB_ASSIGN);
    CASE_2('&', '&', TOKEN_LOG_AND, '=', TOKEN_BIT_AND_ASSIGN);
    CASE_2('|', '|', TOKEN_LOG_OR, '=', TOKEN_BIT_OR_ASSIGN);
    CASE_LONG('<', '<', TOKEN_LSHIFT, '=', TOKEN_LSHIFT_ASSIGN, '=', TOKEN_LTEQ);
    CASE_LONG('>', '>', TOKEN_RSHIFT, '=', TOKEN_RSHIFT_ASSIGN, '=', TOKEN_GTEQ);
#undef CASE_1
#undef CASE_2
#undef CASE_LONG
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': 
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': 
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': 
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        const char* start = stream++;
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
        }
        else {
            assert(res != 0);
            val /= res;
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
        }
        else {
            val -= res;
        }
    }
    return val;
}

int32_t parse_e() {
    return parse_e0();
}

#define assert_token_type(type) (assert(match_token((type))))
#define assert_token_name(x) (assert(str_intern((x)) == token.name && match_token(TOKEN_NAME)))
#define assert_token_float(x) (assert(token.floatval == (x) && match_token(TOKEN_FLOAT)))
#define assert_token_int(x) (assert(token.intval == (x) && match_token(TOKEN_INT)))
#define assert_token_str(x) (assert(strcmp(token.strval, (x)) == 0 && match_token(TOKEN_STR)))
#define assert_token_char(x) (assert(token.intval == (x) && token.mod == TOK_MOD_CHAR && match_token(TOKEN_INT)))
#define assert_token_eof() (assert(is_token(0)))
#define init_stream(src) (stream = (src), next_token())

_lex_test() {
    init_stream("0 032  + 0x111f");
    assert_token_int(0);
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

    init_stream("+ - = += -= %= << >> >>= <= || >");
    assert_token_type('+');
    assert_token_type('-');
    assert_token_type('=');
    assert_token_type(TOKEN_ADD_ASSIGN);
    assert_token_type(TOKEN_SUB_ASSIGN);
    assert_token_type(TOKEN_MOD_ASSIGN);
    assert_token_type(TOKEN_LSHIFT);
    assert_token_type(TOKEN_RSHIFT);
    assert_token_type(TOKEN_RSHIFT_ASSIGN);
    assert_token_type(TOKEN_LTEQ);
    assert_token_type(TOKEN_LOG_OR);
    assert_token_type('>');
    assert_token_eof();

    init_stream("\"sdfasf\" \"hel\nlo\\n world!\"");
    assert_token_str("sdfasf");
    assert_token_str("hel\nlo\n world!");
    assert_token_eof();

    printf("lex test passed\n");
}

#undef assert_token_type
#undef assert_token_int
#undef assert_token_name
#undef assert_token_eof


int32_t parse_src(const char* src) {
    init_stream(src);
#ifdef VERBOSE_PARSE
    printf("Evaluating \"%s\"\n", src);
#endif
    int32_t res = parse_e();
#ifdef VERBOSE_PARSE
    printf("Result     %d\n\n", res);
#endif
    return res;
}

#define TEST_PARSE(expr) (assert(parse_src(#expr) == (expr)))
parse_test() {
    TEST_PARSE(10 + 1);
    TEST_PARSE(2);
    TEST_PARSE(-23423423);
    TEST_PARSE(-(3 - 5));
    TEST_PARSE(4 / 2 + 1);
    TEST_PARSE(-1 * (2 + (3 - 6 / 3)));
    printf("parse test passed\n");
}
#undef TEST_PARSE

void lex_test() {
    _lex_test();
    parse_test();
}