#include "bminor/ast.hpp"
#include "bminor/lexer.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Test failed: " << message << '\n';
        std::exit(1);
    }
}

void test_identifiers_and_keywords() {
    bminor::Lexer lexer("main _value item42 return", "sample.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();

    require(tokens.size() == 5, "identifier and keyword token count");
    require(tokens[0].type == bminor::TokenType::Identifier, "main is an identifier");
    require(tokens[1].type == bminor::TokenType::Identifier, "underscore identifier");
    require(tokens[2].type == bminor::TokenType::Identifier, "identifier with digits");
    require(tokens[3].type == bminor::TokenType::Keyword, "return is a keyword");
    require(tokens[4].type == bminor::TokenType::EndOfFile, "EOF token");
    require(tokens[0].location.filename == "sample.bm", "filename is recorded");
    require(tokens[3].location.column == 20, "column is recorded");
}

void test_whitespace_and_comments() {
    bminor::Lexer lexer("alpha // line\n/* block */ beta", "comments.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();

    require(tokens.size() == 3, "comments are skipped");
    require(tokens[0].lexeme == "alpha", "first identifier survives comments");
    require(tokens[1].lexeme == "beta", "second identifier survives comments");
    require(tokens[1].location.line == 2, "line count after comment");
}

void test_invalid_character_and_unterminated_comment() {
    bminor::Lexer invalid_lexer("name @ next", "invalid.bm");
    const std::vector<bminor::Token> invalid_tokens = invalid_lexer.scan_all();
    require(invalid_tokens[1].type == bminor::TokenType::Invalid, "unknown character is invalid");
    require(invalid_tokens[2].lexeme == "next", "lexer continues after invalid character");

    bminor::Lexer comment_lexer("name /* unfinished", "unfinished.bm");
    const std::vector<bminor::Token> comment_tokens = comment_lexer.scan_all();
    require(comment_tokens[1].type == bminor::TokenType::Invalid, "unterminated comment is invalid");
    require(comment_tokens[1].lexeme == "/* unfinished", "unterminated comment lexeme is retained");
    require(comment_tokens.back().type == bminor::TokenType::EndOfFile, "EOF follows comment error");
}

void test_number_and_quoted_literals() {
    bminor::Lexer lexer("42 \"hello\\n\" 'q' '\\0'", "literals.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();

    require(tokens.size() == 5, "literal token count");
    require(tokens[0].type == bminor::TokenType::IntegerLiteral, "integer literal token");
    require(tokens[0].lexeme == "42", "integer lexeme is preserved");
    require(tokens[1].type == bminor::TokenType::StringLiteral, "string literal token");
    require(tokens[1].lexeme == "\"hello\\n\"", "string escape remains in lexeme");
    require(tokens[2].type == bminor::TokenType::CharacterLiteral, "character literal token");
    require(tokens[3].type == bminor::TokenType::CharacterLiteral, "escaped character literal token");
    require(tokens[4].type == bminor::TokenType::EndOfFile, "EOF follows literals");
}

void test_invalid_and_overlong_literals() {
    bminor::Lexer invalid_chars("'' 'ab' \"open", "bad-literals.bm");
    const std::vector<bminor::Token> invalid_tokens = invalid_chars.scan_all();
    require(invalid_tokens[0].type == bminor::TokenType::Invalid, "empty character literal is invalid");
    require(invalid_tokens[1].type == bminor::TokenType::Invalid, "multi-character literal is invalid");
    require(invalid_tokens[2].type == bminor::TokenType::Invalid, "unterminated string is invalid");

    const std::string long_identifier(257, 'a');
    bminor::Lexer identifier_lexer(long_identifier, "long-name.bm");
    require(identifier_lexer.scan_all()[0].type == bminor::TokenType::Invalid,
            "identifier longer than 256 characters is invalid");

    const std::string long_string = "\"" + std::string(257, 'b') + "\"";
    bminor::Lexer string_lexer(long_string, "long-string.bm");
    require(string_lexer.scan_all()[0].type == bminor::TokenType::Invalid,
            "string longer than 256 characters is invalid");
}

void test_operators_separators_and_maximal_munch() {
    bminor::Lexer lexer(R"(++ + -- - <= =< < >= > == = != ! && || * / % ^ ( ) [ ] { } , ; :)",
                       "symbols.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    const std::vector<std::string> expected_lexemes = {
        "++", "+", "--", "-", "<=", "=<", "<", ">=", ">", "==", "=", "!=", "!",
        "&&", "||", "*", "/", "%", "^", "(", ")", "[", "]", "{", "}", ",", ";", ":"
    };

    require(tokens.size() == expected_lexemes.size() + 1, "symbol token count");
    for (std::size_t i = 0; i < expected_lexemes.size(); ++i) {
        require(tokens[i].lexeme == expected_lexemes[i], "symbol longest-match lexeme");
        if (i < 19) {
            require(tokens[i].type == bminor::TokenType::Operator, "operator token type");
        } else {
            require(tokens[i].type == bminor::TokenType::Separator, "separator token type");
        }
    }
    require(tokens.back().type == bminor::TokenType::EndOfFile, "EOF follows symbols");

    bminor::Lexer invalid_symbols("& |", "invalid-symbols.bm");
    const std::vector<bminor::Token> invalid_tokens = invalid_symbols.scan_all();
    require(invalid_tokens[0].type == bminor::TokenType::Invalid, "single ampersand is invalid");
    require(invalid_tokens[1].type == bminor::TokenType::Invalid, "single vertical bar is invalid");
}

void test_ast_node_foundation() {
    const bminor::SourceLocation expression_location{"tree.bm", 3, 5};
    bminor::AstNodePtr expression = bminor::make_ast_node(
        bminor::AstKind::BinaryExpression, expression_location, "+");
    expression->children.push_back(bminor::make_ast_node(
        bminor::AstKind::NameExpression, {"tree.bm", 3, 5}, "x"));
    expression->children.push_back(bminor::make_ast_node(
        bminor::AstKind::IntegerLiteral, {"tree.bm", 3, 9}, "2"));

    require(expression->kind == bminor::AstKind::BinaryExpression, "AST internal node kind");
    require(expression->children.size() == 2, "binary AST node has two children");
    require(expression->location.line == 3 && expression->location.column == 5,
            "AST node source location");
    require(!expression->type_annotation.has_value(), "AST type annotation starts empty");
    require(expression->children[0]->kind == bminor::AstKind::NameExpression,
            "AST leaf node kind");
    require(std::string(bminor::ast_kind_name(expression->kind)) == "BinaryExpression",
            "AST kind name");
}

}

int main() {
    test_identifiers_and_keywords();
    test_whitespace_and_comments();
    test_invalid_character_and_unterminated_comment();
    test_number_and_quoted_literals();
    test_invalid_and_overlong_literals();
    test_operators_separators_and_maximal_munch();
    test_ast_node_foundation();
    std::cout << "Front-end tests passed.\n";
    return 0;
}
