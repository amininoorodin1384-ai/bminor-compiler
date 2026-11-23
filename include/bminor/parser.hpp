#pragma once

#include "bminor/ast.hpp"
#include "bminor/token.hpp"

#include <cstddef>
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
        AstNodePtr parse_program();
        const std::vector<ParseError>& errors() const;

    private:
        const Token& current() const;
        const Token& previous() const;
        bool at_end() const;
        bool check(TokenType type) const;
        bool check_lexeme(const std::string& lexeme) const;
        bool match_lexeme(const std::string& lexeme);
        const Token& advance();
        const Token& peek(std::size_t distance) const;
        void report(const std::string& message);
        bool expect_lexeme(const std::string& lexeme, const std::string& message);
        bool looks_like_variable_declaration() const;
        void synchronize_statement();
        void synchronize_declaration();

        AstNodePtr parse_declaration();
        AstNodePtr parse_variable_declaration();
        AstNodePtr parse_function_declaration(const Token& name);
        AstNodePtr parse_block();
        AstNodePtr parse_statement();
        AstNodePtr parse_if_statement();
        AstNodePtr parse_for_statement();
        AstNodePtr parse_print_statement();
        AstNodePtr parse_return_statement();
        AstNodePtr parse_type(bool allow_array_size);
        AstNodePtr parse_atomic_type();
        AstNodePtr parse_initializer();
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
