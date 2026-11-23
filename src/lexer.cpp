#include "bminor/lexer.hpp"

#include <optional>
#include <unordered_set>
#include <utility>

namespace bminor {
namespace {

bool is_letter(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool is_identifier_start(char ch) {
    return is_letter(ch) || ch == '_';
}

bool is_identifier_part(char ch) {
    return is_identifier_start(ch) || is_digit(ch);
}

bool is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

bool is_newline(char ch) {
    return ch == '\n' || ch == '\r';
}

bool is_keyword(const std::string& text) {
    static const std::unordered_set<std::string> keywords = {
        "array", "boolean", "char", "else", "false", "for", "function",
        "if", "integer", "print", "return", "string", "true", "void", "while"
    };
    return keywords.find(text) != keywords.end();
}

}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "IDENTIFIER";
        case TokenType::Keyword: return "KEYWORD";
        case TokenType::IntegerLiteral: return "INTEGER_LITERAL";
        case TokenType::StringLiteral: return "STRING_LITERAL";
        case TokenType::CharacterLiteral: return "CHARACTER_LITERAL";
        case TokenType::Invalid: return "INVALID";
        case TokenType::EndOfFile: return "EOF";
    }
    return "UNKNOWN";
}

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

char Lexer::current() const {
    return at_end() ? '\0' : source_[index_];
}

char Lexer::look_ahead(std::size_t distance) const {
    const std::size_t position = index_ + distance;
    return position < source_.size() ? source_[position] : '\0';
}

bool Lexer::at_end() const {
    return index_ >= source_.size();
}

char Lexer::advance() {
    const char ch = source_[index_++];
    if (ch == '\r') {
        ++line_;
        column_ = 1;
    } else if (ch == '\n') {
        if (index_ < 2 || source_[index_ - 2] != '\r') {
            ++line_;
        }
        column_ = 1;
    } else {
        ++column_;
    }
    return ch;
}

std::optional<Token> Lexer::skip_whitespace_and_comments() {
    bool skipping = true;
    while (skipping && !at_end()) {
        if (is_whitespace(current())) {
            advance();
        } else if (current() == '/' && look_ahead() == '/') {
            while (!at_end() && !is_newline(current())) {
                advance();
            }
        } else if (current() == '/' && look_ahead() == '*') {
            const std::size_t start = index_;
            const SourceLocation location{filename_, line_, column_};
            advance();
            advance();
            while (!at_end() && !(current() == '*' && look_ahead() == '/')) {
                advance();
            }
            if (!at_end()) {
                advance();
                advance();
            } else {
                return make_token(TokenType::Invalid, start, location);
            }
        } else {
            skipping = false;
        }
    }
    return std::nullopt;
}

Token Lexer::scan_identifier_or_keyword() {
    const std::size_t start = index_;
    const SourceLocation location{filename_, line_, column_};
    advance();
    while (!at_end() && is_identifier_part(current())) {
        advance();
    }
    const std::string text = source_.substr(start, index_ - start);
    if (text.size() > 256) {
        return Token{TokenType::Invalid, text, location};
    }
    return Token{is_keyword(text) ? TokenType::Keyword : TokenType::Identifier,
                 text, location};
}

Token Lexer::scan_number() {
    const std::size_t start = index_;
    const SourceLocation location{filename_, line_, column_};
    while (!at_end() && is_digit(current())) {
        advance();
    }
    return make_token(TokenType::IntegerLiteral, start, location);
}

Token Lexer::scan_quoted_literal(char quote, TokenType token_type) {
    const std::size_t start = index_;
    const SourceLocation location{filename_, line_, column_};
    advance();

    std::size_t value_length = 0;
    bool valid = true;

    while (!at_end() && current() != quote && !is_newline(current())) {
        if (current() == '\\') {
            advance();
            if (at_end() || is_newline(current())) {
                valid = false;
                break;
            }
            advance();
        } else {
            advance();
        }

        ++value_length;
        if (token_type == TokenType::StringLiteral && value_length > 256) {
            valid = false;
        }
        if (token_type == TokenType::CharacterLiteral && value_length > 1) {
            valid = false;
        }
    }

    if (!at_end() && current() == quote) {
        advance();
        if (token_type == TokenType::CharacterLiteral && value_length != 1) {
            valid = false;
        }
        return make_token(valid ? token_type : TokenType::Invalid, start, location);
    }

    return make_token(TokenType::Invalid, start, location);
}

Token Lexer::make_token(TokenType type, std::size_t start, SourceLocation location) const {
    return Token{type, source_.substr(start, index_ - start), std::move(location)};
}

std::vector<Token> Lexer::scan_all() {
    std::vector<Token> tokens;
    while (!at_end()) {
        const std::optional<Token> comment_error = skip_whitespace_and_comments();
        if (comment_error.has_value()) {
            tokens.push_back(*comment_error);
            break;
        }
        if (at_end()) {
            break;
        }

        const std::size_t start = index_;
        const SourceLocation location{filename_, line_, column_};
        if (is_identifier_start(current())) {
            tokens.push_back(scan_identifier_or_keyword());
        } else if (is_digit(current())) {
            tokens.push_back(scan_number());
        } else if (current() == '"') {
            tokens.push_back(scan_quoted_literal('"', TokenType::StringLiteral));
        } else if (current() == '\'') {
            tokens.push_back(scan_quoted_literal('\'', TokenType::CharacterLiteral));
        } else {
            advance();
            tokens.push_back(make_token(TokenType::Invalid, start, location));
        }
    }

    tokens.push_back(Token{TokenType::EndOfFile, "", SourceLocation{filename_, line_, column_}});
    return tokens;
}

}
