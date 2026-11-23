#pragma once

#include <cstddef>
#include <string>

namespace bminor {

    enum class TokenType {
        Identifier,
        Keyword,
        IntegerLiteral,
        StringLiteral,
        CharacterLiteral,
        Invalid,
        EndOfFile
    };

    struct SourceLocation {
        std::string filename;
        std::size_t line = 1;
        std::size_t column = 1;
    };

    struct Token {
        TokenType type;
        std::string lexeme;
        SourceLocation location;
    };

    const char* token_type_name(TokenType type);

}