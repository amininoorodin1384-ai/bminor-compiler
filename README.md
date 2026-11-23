# B-Minor Compiler Front-End

پروژهٔ آموزشی سه‌فازه برای زبان B-Minor، با پیاده‌سازی C++17 و یک مخزن Git پیوسته.

## وضعیت فعلی

- Lexer شناسه‌ها، کلیدواژه‌ها، فاصله‌ها، commentها، اعداد صحیح، رشته‌ها، کاراکترها، عملگرها و جداکننده‌ها را شناسایی می‌کند.
- اجرای `bminor --tokens <source-file>` توکن‌ها را همراه با موقعیت منبع نمایش می‌دهد.
- آزمون‌های فعلی در `tests/lexer_tests.cpp` هستند.
- Parser، AST، بازیابی خطای نحوی و highlighter هنوز باید پیاده‌سازی شوند.

## ساخت و اجرا

```sh
cmake -S . -B build
cmake --build build
./build/bminor --help
./build/bminor --tokens tests/lexer-smoke.bm
```

در CLion، پروژه را از پوشه‌ای باز کن که `CMakeLists.txt` در آن قرار دارد. برای اجرای آزمون‌ها، هدف `bminor_tests` را انتخاب کن.

## ساختار پروژه

- `include/bminor/` — سرآیندهای Lexer و توکن‌ها
- `src/` — پیاده‌سازی برنامه و Lexer
- `tests/` — آزمون‌ها و فایل‌های نمونهٔ B-Minor
- `docs/lexical-rules.fa.md` — یادداشت موقت قواعد Lexer؛ مشخصات نهایی نیز در همین README تجمیع می‌شود

## قواعد واژگانی

در این بخش `LETTER` یعنی حرف ASCII، `DIGIT` یعنی رقم ده‌دهی، و طول شناسه حداکثر ۲۵۶ بایت است.

```text
LETTER             = [A-Z] | [a-z]
DIGIT              = [0-9]
IDENTIFIER         = (LETTER | _) (LETTER | DIGIT | _){0,255}
INTEGER_LITERAL    = DIGIT+
STRING_LITERAL     = " (ordinary-string-character | escape-sequence)* "
CHARACTER_LITERAL   = ' (ordinary-character | escape-sequence) '
ESCAPE_SEQUENCE     = backslash followed by any non-newline character
LINE_COMMENT        = // followed by characters through end of line
BLOCK_COMMENT       = /* followed by the first subsequent */
WHITESPACE          = space | tab | LF | CR
```

`\n` and `\0` are the newline and null escapes. A backslash followed by another character denotes that character. A string value may contain at most ۲۵۶ characters; each escape sequence counts as one value character. A character literal must denote exactly one character. The Lexer retains the original lexeme, including quotes and escape spelling.

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
comparison_expression    ::= additive_expression
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
- The grammar has not yet been connected to a Parser or AST. It is the approved syntax draft for the next implementation step.
