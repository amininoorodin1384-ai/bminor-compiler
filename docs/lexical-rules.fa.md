# Implemented Lexical Rules

This document records the current Lexer implementation and distinguishes it from the full language specification. Whitespace, comments, identifiers, keywords, integer, string and character literals, operators, separators, and basic lexical errors are implemented.

## Current rules

| Category | Rule |
|---|---|
| Letter | One of `A-Z` or `a-z` |
| Identifier start | A letter or `_` |
| Identifier continuation | A letter, digit `0-9`, or `_` |
| Identifier longer than 256 bytes | Produces an `INVALID` token |
| Integer literal | One or more decimal digits, with no sign; a minus sign is a separate operator |
| String literal | Text enclosed in `"..."`; an escape is read as a backslash followed by the next character; at most 256 value characters |
| Character literal | Exactly one value character enclosed in `'...'`; an escape counts as one value character |
| Operators | `++`, `--`, `-`, `!`, `^`, `*`, `/`, `%`, `+`, `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&`, `||`, `=` |
| Separators | `(`, `)`, `[`, `]`, `{`, `}`, `,`, `;`, `:` |
| Keywords | `array`, `boolean`, `char`, `else`, `false`, `for`, `function`, `if`, `integer`, `print`, `return`, `string`, `true`, `void`, `while` |
| Whitespace | Space, tab, LF, or CR; omitted from the token stream |
| Line comment | From `//` through the end of the line; omitted |
| Block comment | From `/*` through the first subsequent `*/`; omitted |

The Lexer reads a complete word as an identifier first, then checks it against the keyword set. This prevents a keyword from being misclassified as an identifier. Multi-character operators such as `++`, `<=`, `==`, and `&&` are recognized before single-character operators to implement maximal munch.

The B-Minor reference specifies `<=`; the Phase 1 handout uses `=<` as a maximal-munch example. To cover both texts, the Lexer currently accepts `=<` as a single operator and preserves its original lexeme. Unknown characters such as `&` produce `INVALID` tokens. Unterminated block comments, strings, and character literals also produce `INVALID` tokens. Identifiers longer than 256 bytes and strings longer than 256 value characters are invalid. Every token stores its type, lexeme, and source filename/line/column. An `EOF` token is emitted at the end of the input.

## Current limitations

- Identifiers are processed as ASCII bytes; non-ASCII identifiers are outside the current implementation.
- Tests for maximal munch, operator and separator categories, and basic errors are maintained in `tests/lexer_tests.cpp`.
- `=<` is accepted only to honor the Phase 1 handout's explicit example; the standard operator in the B-Minor reference is `<=`.
