# B-Minor Compiler Front-End

An educational, three-phase compiler project for the B-Minor language, implemented in C++17 and developed in one continuous Git repository.

## Current status

- The Lexer recognizes identifiers, keywords, whitespace, comments, integer, string and character literals, operators, and separators.
- `bminor --tokens <source-file>` prints tokens with source locations. `bminor --parse <source-file>` parses a program, reports syntax errors, and prints its AST.
- The recursive-descent Parser builds ASTs for expressions, variable and function declarations, prototypes, blocks, and the core statements. Basic panic-mode error recovery is implemented.
- The syntax highlighter produces ANSI terminal output and standalone HTML/CSS. It uses AST information to distinguish function, variable, and parameter names.
- Unit tests are in `tests/`; test executables are separate CMake targets.

## Build and run

```sh
cmake -S . -B build
cmake --build build
./build/bminor --help
./build/bminor --tokens tests/lexer-smoke.bm
./build/bminor --parse tests/program-smoke.bm
./build/bminor --highlight-ansi tests/program-smoke.bm
./build/bminor --highlight-html tests/program-smoke.bm program-highlight.html
```

In CLion, open the directory containing `CMakeLists.txt`. Select the relevant target (`bminor`, `bminor_tests`, `bminor_parser_tests`, or `bminor_highlighter_tests`) to run the program or a test suite.

## Project structure

- `include/bminor/` — public headers for tokens, AST, Lexer, Parser, and highlighter
- `src/` — application and front-end implementations
- `tests/` — unit tests and B-Minor smoke-test programs
- `docs/lexical-rules.fa.md` — implementation notes for the Lexer

## Lexical rules

In this section, `LETTER` means an ASCII letter, `DIGIT` means a decimal digit, and an identifier is limited to 256 bytes.

```text
LETTER              = [A-Z] | [a-z]
DIGIT               = [0-9]
IDENTIFIER          = (LETTER | _) (LETTER | DIGIT | _){0,255}
INTEGER_LITERAL     = DIGIT+
STRING_LITERAL      = " (ordinary-string-character | escape-sequence)* "
CHARACTER_LITERAL   = ' (ordinary-character | escape-sequence) '
ESCAPE_SEQUENCE     = backslash followed by any non-newline character
LINE_COMMENT        = // followed by characters through end of line
BLOCK_COMMENT       = /* followed by the first subsequent */
WHITESPACE          = space | tab | LF | CR
```

`\n` and `\0` are the newline and null escapes. A backslash followed by another character denotes that character. A string value may contain at most 256 characters; each escape sequence counts as one value character. A character literal must denote exactly one character. The Lexer retains the original lexeme, including quotes and escape spelling.

Keywords are `array`, `boolean`, `char`, `else`, `false`, `for`, `function`, `if`, `integer`, `print`, `return`, `string`, `true`, `void`, and `while`.

Operators are `++`, `--`, unary/binary `-`, `!`, `^`, `*`, `/`, `%`, `+`, `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&`, `||`, and `=`. Separators are `(`, `)`, `[`, `]`, `{`, `}`, `,`, `;`, and `:`. The Lexer uses maximal munch for multi-character operators and checks a complete identifier against the keyword set before returning its token type. Unknown characters and unterminated block comments or literals produce `INVALID` tokens with source locations.

The B-Minor reference uses `<=`; the Phase 1 handout explicitly gives `=<` as a maximal-munch example. To meet both texts, the current Lexer accepts `=<` as one operator token and preserves its original lexeme.

## B-Minor grammar (EBNF)

In the grammar below, `{ X }` means zero or more repetitions and `[ X ]` means an optional part. Uppercase names such as `IDENTIFIER` refer to Lexer token categories.

```ebnf
program                   ::= { declaration } ;

declaration               ::= IDENTIFIER ":" declaration_tail ;
declaration_tail          ::= function_tail
                            | variable_type [ "=" initializer ] ";" ;

function_tail             ::= "function" return_type "(" [ parameter_list ] ")"
                              ( ";" | "=" block ) ;
return_type               ::= atomic_type | "void" ;
parameter_list            ::= parameter { "," parameter } ;
parameter                 ::= IDENTIFIER ":" parameter_type ;
parameter_type            ::= atomic_type
                            | "array" "[" "]" atomic_type ;

variable_declaration      ::= IDENTIFIER ":" variable_type
                              [ "=" initializer ] ";" ;
variable_type             ::= atomic_type
                            | "array" "[" INTEGER_LITERAL "]" atomic_type ;
atomic_type               ::= "integer" | "boolean" | "char" | "string" ;
initializer               ::= expression | array_initializer ;
array_initializer         ::= "{" expression_list "}" ;
expression_list           ::= expression { "," expression } ;

block                     ::= "{" { block_item } "}" ;
block_item                ::= variable_declaration | statement ;
statement                 ::= block
                            | expression ";"
                            | "if" "(" expression ")" statement [ "else" statement ]
                            | "for" "(" [ expression ] ";" [ expression ] ";"
                              [ expression ] ")" statement
                            | "print" expression_list ";"
                            | "return" [ expression ] ";" ;

expression                ::= assignment_expression ;
assignment_expression     ::= logical_or_expression
                              [ "=" assignment_expression ] ;
logical_or_expression     ::= logical_and_expression
                              { "||" logical_and_expression } ;
logical_and_expression    ::= comparison_expression
                              { "&&" comparison_expression } ;
comparison_expression     ::= additive_expression
                              { ( "<" | "<=" | "=<" | ">" | ">=" | "==" | "!=" )
                                additive_expression } ;
additive_expression       ::= multiplicative_expression
                              { ( "+" | "-" ) multiplicative_expression } ;
multiplicative_expression ::= exponent_expression
                              { ( "*" | "/" | "%" ) exponent_expression } ;
exponent_expression       ::= unary_expression [ "^" exponent_expression ] ;
unary_expression          ::= ( "!" | "-" ) unary_expression
                            | postfix_expression ;
postfix_expression        ::= primary_expression
                              { "[" expression "]"
                              | "(" [ argument_list ] ")"
                              | "++"
                              | "--" } ;
argument_list             ::= expression { "," expression } ;
primary_expression        ::= IDENTIFIER
                            | INTEGER_LITERAL
                            | STRING_LITERAL
                            | CHARACTER_LITERAL
                            | "true"
                            | "false"
                            | "(" expression ")" ;
```

### Parser decisions and boundaries

- The selected parser strategy is recursive descent. The expression grammar uses repetitions instead of left recursion; assignment and exponentiation are right-associative in this draft.
- The `while` keyword is recognized by the Lexer, but `while` statements are excluded because the B-Minor reference says the language has no while-loop statement.
- The explicit `=<` spelling is accepted as an alias for the reference's `<=` operator.
- Array variables use a fixed integer size; array parameters use `[]` without a size.
- A complete program's required `main` function, constant-only global initialization, and type compatibility are semantic checks, not grammar productions.
- AST nodes store their kind, source location, value, children, and an optional type annotation that starts empty. The Parser handles expressions, declarations, prototypes and definitions, blocks, and the core statements. The highlighter consumes token and AST information to generate ANSI and HTML output.
