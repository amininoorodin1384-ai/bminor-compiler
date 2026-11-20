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

}

int main() {
    test_identifiers_and_keywords();
    test_whitespace_and_comments();
    test_invalid_character_and_unterminated_comment();
    std::cout << "Lexer tests passed.\n";
    return 0;
}
