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

#include "common.c"
#include "lex.c"
#include "ast.c"
#include "print.c"
#include "parse.c"
#include "test.c"

int main(int argc, char** argv) {
    run_tests();
    getchar();
    return 0;
}