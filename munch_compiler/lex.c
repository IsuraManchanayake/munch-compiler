const char* kwrd_if;
const char* kwrd_else;
const char* kwrd_for;
const char* kwrd_while;
const char* kwrd_do;
const char* kwrd_switch;
const char* kwrd_case;
const char* kwrd_default;
const char* kwrd_break;
const char* kwrd_continue;
const char* kwrd_return;
const char* kwrd_struct;
const char* kwrd_union;
const char* kwrd_enum;
const char* kwrd_const;
const char* kwrd_sizeof;
const char* kwrd_var;
const char* kwrd_func;
const char* kwrd_typedef;
const char* kwrd_true;
const char* kwrd_false;

const char** keywords;
const char* first_kwrd;
const char* last_kwrd;

#define INIT_KEYWORD(k) kwrd_##k = str_intern(#k); buf_push(keywords, kwrd_##k)

void init_keywords() {
    static bool keyword_inited = false;
    if (keyword_inited) {
        return;
    }
    INIT_KEYWORD(if);
    INIT_KEYWORD(else);
    INIT_KEYWORD(for);
    INIT_KEYWORD(while);
    INIT_KEYWORD(do);
    INIT_KEYWORD(switch);
    INIT_KEYWORD(case);
    INIT_KEYWORD(default);
    INIT_KEYWORD(break);
    INIT_KEYWORD(continue);
    INIT_KEYWORD(return);
    INIT_KEYWORD(struct);
    INIT_KEYWORD(union);
    INIT_KEYWORD(enum);
    INIT_KEYWORD(const);
    INIT_KEYWORD(sizeof);
    INIT_KEYWORD(var);
    INIT_KEYWORD(func);
    INIT_KEYWORD(typedef);
    INIT_KEYWORD(true);
    INIT_KEYWORD(false);
    first_kwrd = kwrd_if;
    last_kwrd = kwrd_false;
    keyword_inited = true;
}

#undef INIT_KEYWORD

typedef enum TokenType {
    TOKEN_EOF = 0,
    // do not define in between. allocated for one char operators
    TOKEN_LAST_CHAR = 127,
    TOKEN_KEYWORD,
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
const char* src_start;
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
            basic_syntax_error("Invalid integer literal %d for base %d", digit, base);
            digit = 0;
        }
        if (val > (UINT64_MAX - digit) / base) {
            basic_syntax_error("Integer literal overflow");
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
            basic_syntax_error("Expected digit or sign after exponent in the float literal. Found '%c'", *stream);
        }
        if (!isdigit(*stream)) stream++;
        while (isdigit(*stream)) stream++;
    }
    const char* end = stream;
    if (end - start == 1) {
        token.type = '.';
        return;
    }
    double val = strtod(start, NULL);
    if (val == HUGE_VAL || val == -HUGE_VAL) {
        basic_syntax_error("Float literal overflow");
    }
    token.type = TOKEN_FLOAT;
    token.floatval = val;
}

const char esc_to_char[256] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['f'] = '\f',
    ['0'] = '\0',
    ['"'] = '"',
    ['\''] = '\'',
    ['\\'] = '\\'
};

void scan_char() {
    stream++;
    char val;
    if (*stream == '\'') {
        basic_syntax_error("Char literal should be of length 1");
    }
    if (*stream == '\n' || *stream == '\r' || *stream == '\a' || 
        *stream == '\b' || *stream == '\f' || *stream == '\v') {
        basic_syntax_error("Char literals cannot have escape characters inside quotes. Found <ASCII %d>.", (int)(*stream));
    }
    if (*stream == '\\') {
        stream++;
        val = *stream == '"' ? 0 : esc_to_char[*stream];
        if (val == 0 && *stream != '0') {
            basic_syntax_error("Undefined escape char literal");
        }
        stream++;
    }
    else {
        val = *stream;
        stream++;
    }
    if(*stream != '\'') {
        basic_syntax_error("Char literal should be ended with \'. Found '%c'.", *stream);
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
            basic_syntax_error("String literals cannot have escape characters inside quotes. Found <ASCII %d>.", (int)(*stream));
        }
        if (*stream == '\n' || *stream == '\r') {
            buf_push(str_buf, *stream);
            stream++;
        }
        if (*stream == '\\') {
            stream++;
            val = *stream == '\'' ? 0 : esc_to_char[*stream];
            if (val == 0 && *stream != '0') {
                basic_syntax_error("Undefined escape char literal");
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
        basic_syntax_error("String literal should be ended with '\"'. Found %c", *stream);
    }
    buf_push(str_buf, 0);
    stream++;
    token.type = TOKEN_STR;
    token.strval = str_buf;
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
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': 
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': 
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': 
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        const char* start = stream++;
        while (isalnum(*stream) || *stream == '_') stream++;
        token.name = str_intern_range(start, stream);
        token.type = (token.name >= first_kwrd && token.name <= last_kwrd) ? TOKEN_KEYWORD : TOKEN_NAME;
        break;
    }
    default:
        token.type = *stream++;
    }
    token.end = stream;
}

#undef CASE_1
#undef CASE_2
#undef CASE_LONG

void init_stream(const char* src) {
    src_start = src;
    stream = src;
    next_token();
}

const char* op_to_str(TokenType op) {
    switch (op) {
    case TOKEN_INC:
        return "++";
    case TOKEN_DEC:
        return "--";
    case TOKEN_LOG_AND:
        return "&&";
    case TOKEN_LOG_OR:
        return "||";
    case TOKEN_LSHIFT:
        return "<<";
    case TOKEN_RSHIFT:
        return ">>";
    case TOKEN_EQ:
        return "==";
    case TOKEN_NEQ:
        return "!=";
    case TOKEN_LTEQ:
        return "<=";
    case TOKEN_GTEQ:
        return ">=";
    case TOKEN_COLON_ASSIGN:
        return ":=";
    case TOKEN_ADD_ASSIGN:
        return "+=";
    case TOKEN_SUB_ASSIGN:
        return "-=";
    case TOKEN_DIV_ASSIGN:
        return "/=";
    case TOKEN_MUL_ASSIGN:
        return "*=";
    case TOKEN_MOD_ASSIGN:
        return "%=";
    case TOKEN_BIT_AND_ASSIGN:
        return "&=";
    case TOKEN_BIT_OR_ASSIGN:
        return "|=";
    case TOKEN_BIT_XOR_ASSIGN:
        return "^=";
    case TOKEN_LSHIFT_ASSIGN:
        return "<<=";
    case TOKEN_RSHIFT_ASSIGN:
        return ">>=";
    }
    static char op_buf[2];
    op_buf[0] = op;
    op_buf[1] = 0;
    return op_buf;
}

const char* token_to_str(Token tok) {
    static char buf[256];
    switch (tok.type) {
    case TOKEN_EOF:
        sprintf_s(buf, sizeof(buf), "<TOKEN_EOF>");
        break;
    case TOKEN_LAST_CHAR:
        sprintf_s(buf, sizeof(buf), "<TOKEN_LAST_CHAR>");
        break;
    case TOKEN_KEYWORD:
        sprintf_s(buf, sizeof(buf), "<KEYWORD %s>", tok.name);
        break;
    case TOKEN_INT:
        sprintf_s(buf, sizeof(buf), tok.mod == TOK_MOD_CHAR ? "<CHAR %c>" : "<INT %d>", tok.intval);
        break;
    case TOKEN_FLOAT:
        sprintf_s(buf, sizeof(buf), "<FLOAT %f>", tok.floatval);
        break;
    case TOKEN_STR:
        sprintf_s(buf, sizeof(buf), "<STR %s>", tok.strval);
        break;
    case TOKEN_NAME:
        sprintf_s(buf, sizeof(buf), "<NAME %s>", tok.name);
        break;
    default:
        if (tok.type >= TOKEN_INC && tok.type <= TOKEN_RSHIFT_ASSIGN) {
            sprintf_s(buf, sizeof(buf), "<OP %s>", op_to_str(tok.type));
        }
        else if (tok.type < 127) {
            sprintf_s(buf, sizeof(buf), "<%c>", tok.type);
        }
        else {
            sprintf_s(buf, sizeof(buf), "<ASCII %d(%c)>", tok.type, tok.type);
        }
    }
    return buf;
}

const char* tokentype_to_str(TokenType type) {
    static char buf[256];
    switch (type) {
    case TOKEN_LAST_CHAR:
        sprintf_s(buf, sizeof(buf), "TOKEN_LAST_CHAR");
        break;
    case TOKEN_INT:
        sprintf_s(buf, sizeof(buf), "TOKEN_INT");
        break;
    case TOKEN_FLOAT:
        sprintf_s(buf, sizeof(buf), "TOKEN_FLOAT");
        break;
    case TOKEN_STR:
        sprintf_s(buf, sizeof(buf), "TOKEN_STR");
        break;
    case TOKEN_NAME:
        sprintf_s(buf, sizeof(buf), "TOKEN_NAME");
        break;
    case TOKEN_KEYWORD:
        sprintf_s(buf, sizeof(buf), "TOKEN_KEYWORD");
        break;
    case TOKEN_EOF:
        sprintf_s(buf, sizeof(buf), "TOKEN_EOF");
        break;
    default:
        if (type < 128 && isprint(type)) {
            sprintf_s(buf, sizeof(buf), "%c", type);
        }
        else if (type < TOKEN_RSHIFT_ASSIGN) {
            sprintf_s(buf, sizeof(buf), "\"%s\"", op_to_str(type));
        }
        else {
            sprintf_s(buf, sizeof(buf), "<ASCII %d(%c)>", type, type);
        }
    }
    return buf;
}

#define ERROR_DISPLAY_WIDTH 20

void show_error_token() {
    size_t curr = token.start - src_start;
    size_t left = max(curr - ERROR_DISPLAY_WIDTH / 2, 0);
    size_t len = (left + ERROR_DISPLAY_WIDTH) % (strlen(src_start)) - left;
    printf("%.*s\n", len, src_start + left);
    size_t err_pos = curr - left;
    if (err_pos) {
        char* ss = malloc(sizeof(char) * (err_pos + 1));
        memset(ss, ' ', sizeof(char) * err_pos);
        ss[err_pos] = 0;
        printf("%s^\n", ss);
        free(ss);
    }
    else {
        printf("^\n");
    }
}

#define syntax_error(fmt, ...) \
    do { \
        show_error_token(); \
        basic_syntax_error(fmt, __VA_ARGS__); \
    } while(0)

inline bool is_token(TokenType type) {
    return token.type == type;
}

inline bool is_token_name(const char* name) {
    return token.type == TOKEN_NAME && token.name == name;
}

inline bool is_keyword(const char* name) {
    return token.type == TOKEN_KEYWORD && token.name == name;
}

inline bool match_token(TokenType type) {
    if (is_token(type)) {
        next_token();
        return true;
    }
    return false;
}

// NOTE: name should be intern str
inline bool match_keyword(const char* name) {
    if (is_keyword(name)) {
        next_token();
        return true;
    }
    return false;
}

inline bool expect_token(TokenType type) {
    if (is_token(type)) {
        next_token();
        return true;
    }
    else {
        show_error_token();
        fatal("Expected token %s. Found %s.", tokentype_to_str(type), token_to_str(token));
        return false;
    }
}

inline bool expect_keyword(const char* name) {
    if (is_keyword(name)) {
        next_token();
        return true;
    }
    else {
        show_error_token();
        fatal("Expected keyword \"%s\". Found \"%s\".", name, token_to_str(token));
        return false;
    }
}

inline bool is_literal() {
    return (token.type >= TOKEN_INT && token.type <= TOKEN_STR) || (token.type == TOKEN_KEYWORD && (token.name == kwrd_true || token.name == kwrd_false));
}

inline bool is_cmp_op() {
    return token.type == '<' || token.type == '>' || (token.type >= TOKEN_EQ && token.type <= TOKEN_GTEQ);
}

inline bool is_shift_op() {
    return token.type == TOKEN_LSHIFT || token.type == TOKEN_RSHIFT;
}

inline bool is_add_op() {
    return token.type == '+' || token.type == '-';
}

inline bool is_mul_op() {
    return token.type == '*' || token.type == '/' || token.type == '%';
}

inline bool is_unary_op() {
    return token.type == '-' || token.type == '+' || token.type == '~'
        || token.type == '&' || token.type == '*' || token.type == '!'
        || token.type == TOKEN_INC || token.type == TOKEN_DEC;
}

inline bool is_assign_op() {
    return token.type == '=' || (token.type >= TOKEN_COLON_ASSIGN && token.type <= TOKEN_RSHIFT_ASSIGN);
}

inline bool expect_assign_op() {
    if (is_assign_op()) {
        next_token();
        return true;
    }
    else {
        show_error_token();
        fatal("Expected an assign operator. Found %s", tokentype_to_str(token.type));
    }
}

#define assert_token_type(type) (assert(match_token((type))))
#define assert_token_name(x) (assert(str_intern((x)) == token.name && match_token(TOKEN_NAME)))
#define assert_token_keyword(x) (assert(match_keyword(str_intern(x))))
#define assert_token_float(x) (assert(token.floatval == (x) && match_token(TOKEN_FLOAT)))
#define assert_token_int(x) (assert(token.intval == (x) && match_token(TOKEN_INT)))
#define assert_token_str(x) (assert(strcmp(token.strval, (x)) == 0 && match_token(TOKEN_STR)))
#define assert_token_char(x) (assert(token.intval == (x) && token.mod == TOK_MOD_CHAR && match_token(TOKEN_INT)))
#define assert_token_eof() (assert(is_token(0)))

lex_test() {
    printf("----- lex.c -----\n");

    init_keywords();

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

    init_stream("\"a\" \"\" \"'\" \"\\\"\" '\"' '\\''");
    assert_token_str("a");
    assert_token_str("");
    assert_token_str("'");
    assert_token_str("\"");
    assert_token_int('"');
    assert_token_int('\'');

    init_stream("case '2':");
    assert_token_keyword(kwrd_case);
    assert_token_char('2');
    assert_token_type(':');
    assert_token_eof();

    init_stream("const pi = 3.14");
    assert_token_keyword(kwrd_const);
    assert_token_name("pi");
    assert_token_type('=');
    assert_token_float(3.14);

    printf("lex test passed\n");
}

#undef assert_token_type
#undef assert_token_name
#undef assert_token_keyword
#undef assert_token_float
#undef assert_token_int
#undef assert_token_str
#undef assert_token_char
#undef assert_token_eof
