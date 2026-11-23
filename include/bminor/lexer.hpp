#pragma once

#include "bminor/token.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bminor {

    class Lexer {
    public:
        Lexer(std::string source, std::string filename);
        std::vector<Token> scan_all();

    private:
        char current() const;
        char look_ahead(std::size_t distance = 1) const;
        bool at_end() const;
        char advance();
        std::optional<Token> skip_whitespace_and_comments();
        Token scan_identifier_or_keyword();
        Token scan_number();
        Token scan_quoted_literal(char quote, TokenType token_type);
        Token scan_symbol();
        Token make_token(TokenType type, std::size_t start, SourceLocation location) const;

        std::string source_;
        std::string filename_;
        std::size_t index_ = 0;
        std::size_t line_ = 1;
        std::size_t column_ = 1;
    };

}