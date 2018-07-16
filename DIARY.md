# Diary

## 13 Jul 2018 4 AM

- Stretchy buffer, intern string implemented
- Basic lexing for integers, identifiers and symbols implemented without any grammar
- Tokens are printed for given sequence of chars

## 14 Jul 2018 3 AM

- intermediate whitespace elemination
- int, double, char, string parsing
- int supports decimal, hexadecimal, octal and binary literals
- int, double are checked for overflow
- char supports escape char literals
- strings supports multiline (CR/LF) str literals
- incompleted virtual machine execution (prolly gonna abandon my implementation)

## 15 Jul 2018 5 AM

- vm code is removed
- munch.c splitted to ast.c, common.c, lex.c, print.c, test.c and main.c
- syntax context free grammar is noted down (munch_compiler/grammar.txt)
- type, declaration, expression and statement data structures are added
- type, expression pretty printer are added

## 16 July 2018 2 AM

- implicit stretchy buffer array fields of ast structures are removed and sizes are introduced
- typespec and expr constructors are completed
- pretty printer issues are fixed
- ast tests passing 

## 17 July 2018 4 AM

- parallel data structures for all types, declarations, statements and expressions are added
- completed constructors for all sub types of asts
- pretty printer for declarations without indentation is added
- TODO pragma warning message added
