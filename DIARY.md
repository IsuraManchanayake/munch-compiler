# Diary

## 28th November 2018 10 PM

- warnings about variable/function usages (set without usage, use without setting, no use nor set) is added

## 9th November 2018 1 AM

- non trivial constant folding and the bug of var declaration expressions are fixed at the same time
- non const expressions for variable declarations are not allowed at the global level by the C compiler
- therefore, foldable arithmetic expressions are evaluated and symbols of compound expressions are simply replaced by their definitions (only var and const) at the C code generation stage.

## 4th November 2018 2 AM

- all linear searches are replaced by hash maps (fnv-1a for both single and multiple data blocks with quadratic probing)
    - string interning
    - global entity table
    - type tables which used hash consing
- 0.5 secs for the previous 2MB source compilation
- 30 secs for 41 MB source
- resolved constant values are used in constant decl C code generation instead of whole expressions
- forgotten typedefs are added to C code generation
- function entities are added to bottom of definitions in C code generation due to possible local entity resolving errors
- a major bug in scope of local parameters is fixed
- rand64 using 64 bit Mersenne's twister is created initially for hash generation which possibly be removed in a later stage
- types are completed after resolving them  

## 25 October 2018 12 AM

- 2 MB source compilation as a torture test is added (elapsed time 6-7 seconds when compiled with -O3) - performance can be further increased by using hashing and better data structures (TODO)
- error reporting (syntax errors, resolve errors) with line numbers and source path is added
- incompleted enum const resolving and C code generation are completed
- single line and multiline comments are added as a feature
- size of expr is added in resolver
- a minor buf in typespec resolving is fixed
- a minor bug in array size is fixed
- bugs in statement var declaration is fixed
- a minor bug in do while loop parsing is fixed
- a minor bug in block statement C generation is fixed

## 23 October 2018 2 AM

- an end to end workflow is created (compiling a given input file path and write the generated C code to a file)
- all gcc pedantic warnings are fixed except expecting at least one arguments for variadic macros, non enumerables in case values
- \_\_VA_ARGS\_\_ are replaced with ##\_\_VA_ARGS\_\_
- gen_ functions for different types of ASTs are moved to different sub gen_ functions
- type expectancy for compound literals are moved as a parameter to the gen_expr_compound function
- all sprintf_s are replaced with buf_printf

## 1 October 2018 2 AM

- initializing statements are added (e.g. i := 0 where i variable is declared at the moment)
- for statement inits are updated correspondingly
- C code generation for statements are completed without complete testing

## 28 September 2018 1 AM

- C code generation for decls, exprs are completed

## 20 September 2018 1 AM

- C code generation for some of the decl types are completed
- full source stream parsing is added
- DeclSet type is introduced to store all declarations
- Type is included as a field in TypeSpec
- a minor bug in keyword tokens is fixed

## 17 September 2018 2 AM

- C code generation is started
    - Type to C decl is completed
- corresponding Type is included in Expr types (assigned in the resolving stage)
- buf_printf and strf functions are added
- is_decl_keyword is optimized
- empty function return types are filled with void type
- few bugs are fixed local entity resolving

## 06 September 2018 3 AM

- local declarations are added (ast, printing, parsing and not resolving)
- local entity resolving is started
- resolving statements of function blocks is added (tests yet to be run) 

## 04 September 2018 2 AM

- array and struct initializers with specific names and indices are added (ast, printing, parsing and resolving)
- few mistakes are fixed in DIARY.md to preserve consistency

## 31 August 2018 2 AM

- cast expr parsing, resolving is added
- void, char in-built types are added
- temporary fix for str and float literals are added
- minor bug is fixed in add expr parsing

## 30 August 2018 2 AM

- resolve.c code are refactored into more readable code by segementing the code to seperate functions
- a bug in func entity resolving is fixed
- ternary, call expr resolving is completed
- built-in keyword PI is introduced
- built-in constants (true, false, PI) are added into the resolving process
- inline structs are replaced by explicit types in the Type definition
- minor bug is fixed in func typespec printing
- all empty function declaration parameters ("()") are changed to void ("(void)")

## 29 August 2018 1 AM

- enum constants are resolved as constants
- enum types are resolved as int typedefs
- compund literal resolving with expected types is completed
- minor bug is fixed in compound literal parsing

## 28 August 2018 3 AM

- first working version of the resolve.c (YAY!! FINALLY!) is created in my own way
- address operator is added as a pre unary operator
- minor bug is fixed in var declaration printing

## 30 July 2018 - 27 August 2018

- minor changes applied
    - tabs are replaced with spaces
    - is_between, is_token_between macro is added
    - minor order changes of functions are applied

## 29 July 2018 4 AM

- resolving decls using entities is partially completed

## 28 July 2018 4 AM

- decl, typespec, expr ordering are added
- builtin sizeof exprs and types is added

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
- '.' is added as a token. previously it is a float literal error
- error in stmnt_ifelseif is fixed
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

- an enum item can has an expression (a const expr which will eventually evaluated to an int val) instead of int value
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
