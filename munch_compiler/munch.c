#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

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

typedef struct InternStr {
	size_t len;
	const char* str;
} InternStr;

static InternStr* interns;

const char* str_intern_range(const char* start, const char* end) {
	size_t len = end - start;
	for (size_t i = 0; i < buf_len(interns); i++) {
		if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0) {
			return interns[i].str;
		}
	}
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
	TOKEN_INT = 128,
	TOKEN_NAME
} TokenType;

typedef struct Token {
	TokenType type;
	const char* start;
	const char* end;
	union {
		int64_t intval;
		const char* name;
	};
	char* strval;
} Token;

Token token;
const char* stream;

void next_token() {
	switch (*stream) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': {
		uint64_t val = 0;
		while (isdigit(*stream)) val = val * 10 + (*stream++ - '0');
		token.type = TOKEN_INT;
		token.intval = val;
		break;
	}
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
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
	assert(buf_len(buf) == NULL);
}

void print_token(Token token) {
	switch (token.type) {
	case TOKEN_INT:
		printf("TOKEN: TOKEN_INT %llu\n", token.intval);
		break;
	case TOKEN_NAME:
		printf("TOKEN: TOKEN_NAME %s %0X\n", token.name, (int)token.name);
		break;
	default:
		printf("TOKEN: %c\n", token.type);
	}
}

lex_test() {
	char* source = "x-3421+(x+y)*z";
	stream = source;
	next_token();
	while (token.type) {
		print_token(token);
		next_token();
	}
}

intern_str_test() {
	char* a = "hi";
	char* b = "hi";
	assert(a == b);
	assert(str_intern(a) == str_intern(b));
	assert(str_intern(a) != str_intern("sdf"));
}

int main(int argc, char** argv) {
	buf_test();
	lex_test();
	intern_str_test();
	getchar();
	return 0;
}
