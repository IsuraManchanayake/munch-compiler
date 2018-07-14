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
- Type, declaration, expression and statement data structures are added
- Type, expression pretty printer are added
