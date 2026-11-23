#pragma once

#include "bminor/ast.hpp"
#include "bminor/token.hpp"

#include <string>
#include <vector>

namespace bminor {

    struct ParseError {
        SourceLocation location;
        std::string message;
    };

    class Parser {
    public:
        explicit Parser(std::vector<Token> tokens);
        AstNodePtr parse_expression();
        const std::vector<ParseError>& errors() const;

    private:
        const Token& current() const;
        const Token& previous() const;
        bool at_end() const;
        bool check(TokenType type) const;
        bool check_lexeme(const std::string& lexeme) const;
        bool match_lexeme(const std::string& lexeme);
        const Token& advance();
        void report(const std::string& message);

        AstNodePtr parse_assignment();
        AstNodePtr parse_logical_or();
        AstNodePtr parse_logical_and();
        AstNodePtr parse_comparison();
        AstNodePtr parse_additive();
        AstNodePtr parse_multiplicative();
        AstNodePtr parse_exponent();
        AstNodePtr parse_unary();
        AstNodePtr parse_postfix();
        AstNodePtr parse_primary();

        std::vector<Token> tokens_;
        std::size_t index_ = 0;
        std::vector<ParseError> errors_;
    };

}
