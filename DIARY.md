# Diary

## 27 July 2018 3 AM

- hash consing for ptr, array and func types is added
- array type attributes of asts are duplicated
- arena_free is added
- void return parsing is added

## 26 July 2018 4 AM

- symbol resolver is added (for order dependencies in declarations)

## 25 July 2018 4 AM

- bugs in char and string literal token parsing are fixed
- token and token type to string methods are added
- true/false keywords parsing as literals is added
- empty statements (consecutive semicolons) parsing is added
- strings are printed along with escape characters

## 24 July 2018 4 AM

- custom exit codes are added for memory allocation errors
- error token displaying (cool!) method is added
- error in is_shift_op is fixed
- '.' is added as a token. previously it was a float literal error
- error in stmnt_ifelseif was fixed
- new parsing features are added
    - optional trailing ',' token for enum or compound expression lists
    - post increment/decrement feature
    - simple statements (assignment/post increment or decrement/expression)
    - list of simple statements feature for 'for' loop init and update statements
    - semicolons at statement endings)
- unions and structs fields are seperated by ';' instead of commas
- new passing test cases for parsing is added
- several minor bugs in print.c are fixed

## 23 July 2018 2 AM

- first version of the parser is done (yay!)
- current tests are passed (more tests have to be written)
- several bugs in previous parse functions are fixed
- a bug in is_literal() is fixed
- optional exprs are checked for nullity in print functions 

## 22 July 2018 1 AM

- expr and stmnt parsers are completed
- init and update of for stmnts are changed to Stmnt** instead of Expr** 
- several token type checkers are added (e.g. is_literal, is_cmp_op, expect_keyword etc.)

## 21 July 2018 4 AM

- an enum item can have an expression (a const expr which will eventually evaluated to an int val) instead of int value
- decl parsing is done
- typespec, expr, stmnt are on the way

## 20 July 2018 2 AM

- arena allocation is used by intern strs
- keywords are identified as seperate tokens
- parser development is started
- stretchy buffer malloc, realloc replaced with xmalloc, xrealloc
- ast duplication is added

## 19 July 2018 3 AM

- (cool) arena allocation is added
- asts allocated with arena allocations pass tests
- set of keywords added

## 18 July 2018 4 AM

- all ast pretty printers are completed now with new line indentations
- example fibonacci test is written for ast printer
- statement bodies of Stmnt sub types (ifelseif, switch, for, while) is replaced with BlockStmnt
- func body is added inside FuncDecl
- ast print methods are moved to print.c
- parse.c is added
- order of DIARY.md is changed reverse (newest first)

## 17 July 2018 4 AM

- parallel data structures for all types, declarations, statements and expressions are added
- completed constructors for all sub types of asts
- pretty printer for declarations without indentation is added
- TODO pragma warning message added

## 16 July 2018 2 AM

- implicit stretchy buffer array fields of ast structures are removed and sizes are introduced
- typespec and expr constructors are completed
- pretty printer issues are fixed
- ast tests passing 

## 15 Jul 2018 5 AM

- vm code is removed
- munch.c splitted to ast.c, common.c, lex.c, print.c, test.c and main.c
- syntax context free grammar is noted down (munch_compiler/grammar.txt)
- type, declaration, expression and statement data structures are added
- type, expression pretty printer are added

## 14 Jul 2018 3 AM

- intermediate whitespace elemination
- int, double, char, string parsing
- int supports decimal, hexadecimal, octal and binary literals
- int, double are checked for overflow
- char supports escape char literals
- strings supports multiline (CR/LF) str literals
- incompleted virtual machine execution (prolly gonna abandon my implementation)

## 13 Jul 2018 4 AM

- Stretchy buffer, intern string implemented
- Basic lexing for integers, identifiers and symbols implemented without any grammar
- Tokens are printed for given sequence of chars
