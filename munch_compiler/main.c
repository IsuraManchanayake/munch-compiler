#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "rand.c"
#include "common.c"
#include "lex.c"
#include "ast.c"
#include "print.c"
#include "parse.c"
#include "resolve.c"
#include "gen.c"
#include "munch.c"
#include "test.c"

int main(int argc, char** argv) {
    //run_tests();
    //munch_test();
    //return 0;
    return munch_main(argc, argv);
}
