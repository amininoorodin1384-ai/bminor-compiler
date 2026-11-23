#include "bminor/parser.hpp"

#include <utility>

namespace bminor {
namespace {

AstNodePtr make_unary_node(const Token& operation, AstNodePtr operand) {
    AstNodePtr node = make_ast_node(AstKind::UnaryExpression, operation.location, operation.lexeme);
    node->children.push_back(std::move(operand));
    return node;
}

AstNodePtr make_binary_node(AstKind kind, const Token& operation,
                            AstNodePtr left, AstNodePtr right) {
    AstNodePtr node = make_ast_node(kind, operation.location, operation.lexeme);
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
}

bool is_comparison_operator(const std::string& lexeme) {
    return lexeme == "<" || lexeme == "<=" || lexeme == "=<" || lexeme == ">" ||
           lexeme == ">=" || lexeme == "==" || lexeme == "!=";
}

}

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    if (tokens_.empty()) {
        tokens_.push_back(Token{TokenType::EndOfFile, "", SourceLocation{}});
    }
}

const Token& Parser::current() const {
    return index_ < tokens_.size() ? tokens_[index_] : tokens_.back();
}

const Token& Parser::previous() const {
    return index_ == 0 ? tokens_.front() : tokens_[index_ - 1];
}

bool Parser::at_end() const {
    return current().type == TokenType::EndOfFile;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::check_lexeme(const std::string& lexeme) const {
    return !at_end() && current().lexeme == lexeme;
}

bool Parser::match_lexeme(const std::string& lexeme) {
    if (!check_lexeme(lexeme)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::advance() {
    if (!at_end()) {
        ++index_;
    }
    return previous();
}

void Parser::report(const std::string& message) {
    errors_.push_back(ParseError{current().location, message});
}

const std::vector<ParseError>& Parser::errors() const {
    return errors_;
}

AstNodePtr Parser::parse_expression() {
    AstNodePtr expression = parse_assignment();
    if (!at_end()) {
        report("Unexpected token after expression.");
    }
    return expression;
}

AstNodePtr Parser::parse_assignment() {
    AstNodePtr left = parse_logical_or();
    if (match_lexeme("=")) {
        const Token operation = previous();
        AstNodePtr right = parse_assignment();
        if (!left || !right) {
            return nullptr;
        }
        return make_binary_node(AstKind::AssignmentExpression, operation,
                                std::move(left), std::move(right));
    }
    return left;
}

AstNodePtr Parser::parse_logical_or() {
    AstNodePtr expression = parse_logical_and();
    while (match_lexeme("||")) {
        const Token operation = previous();
        AstNodePtr right = parse_logical_and();
        if (!expression || !right) {
            return nullptr;
        }
        expression = make_binary_node(AstKind::BinaryExpression, operation,
                                      std::move(expression), std::move(right));
    }
    return expression;
}

AstNodePtr Parser::parse_logical_and() {
    AstNodePtr expression = parse_comparison();
    while (match_lexeme("&&")) {
        const Token operation = previous();
        AstNodePtr right = parse_comparison();
        if (!expression || !right) {
            return nullptr;
        }
        expression = make_binary_node(AstKind::BinaryExpression, operation,
                                      std::move(expression), std::move(right));
    }
    return expression;
}

AstNodePtr Parser::parse_comparison() {
    AstNodePtr expression = parse_additive();
    while (!at_end() && is_comparison_operator(current().lexeme)) {
        const Token operation = advance();
        AstNodePtr right = parse_additive();
        if (!expression || !right) {
            return nullptr;
        }
        expression = make_binary_node(AstKind::BinaryExpression, operation,
                                      std::move(expression), std::move(right));
    }
    return expression;
}

AstNodePtr Parser::parse_additive() {
    AstNodePtr expression = parse_multiplicative();
    while (check_lexeme("+") || check_lexeme("-")) {
        const Token operation = advance();
        AstNodePtr right = parse_multiplicative();
        if (!expression || !right) {
            return nullptr;
        }
        expression = make_binary_node(AstKind::BinaryExpression, operation,
                                      std::move(expression), std::move(right));
    }
    return expression;
}

AstNodePtr Parser::parse_multiplicative() {
    AstNodePtr expression = parse_exponent();
    while (check_lexeme("*") || check_lexeme("/") || check_lexeme("%")) {
        const Token operation = advance();
        AstNodePtr right = parse_exponent();
        if (!expression || !right) {
            return nullptr;
        }
        expression = make_binary_node(AstKind::BinaryExpression, operation,
                                      std::move(expression), std::move(right));
    }
    return expression;
}

AstNodePtr Parser::parse_exponent() {
    AstNodePtr left = parse_unary();
    if (match_lexeme("^")) {
        const Token operation = previous();
        AstNodePtr right = parse_exponent();
        if (!left || !right) {
            return nullptr;
        }
        return make_binary_node(AstKind::BinaryExpression, operation,
                                std::move(left), std::move(right));
    }
    return left;
}

AstNodePtr Parser::parse_unary() {
    if (check_lexeme("!") || check_lexeme("-")) {
        const Token operation = advance();
        AstNodePtr operand = parse_unary();
        if (!operand) {
            return nullptr;
        }
        return make_unary_node(operation, std::move(operand));
    }
    return parse_postfix();
}

AstNodePtr Parser::parse_postfix() {
    AstNodePtr expression = parse_primary();
    while (expression && !at_end()) {
        if (match_lexeme("[")) {
            AstNodePtr index = parse_assignment();
            if (!match_lexeme("]")) {
                report("Expected ']' after array index.");
                return nullptr;
            }
            if (!index) {
                return nullptr;
            }
            AstNodePtr node = make_ast_node(AstKind::IndexExpression, expression->location);
            node->children.push_back(std::move(expression));
            node->children.push_back(std::move(index));
            expression = std::move(node);
        } else if (match_lexeme("(")) {
            AstNodePtr call = make_ast_node(AstKind::CallExpression, expression->location);
            call->children.push_back(std::move(expression));
            if (!check_lexeme(")")) {
                call->children.push_back(parse_assignment());
                while (match_lexeme(",")) {
                    call->children.push_back(parse_assignment());
                }
            }
            if (!match_lexeme(")")) {
                report("Expected ')' after function arguments.");
                return nullptr;
            }
            for (const AstNodePtr& argument : call->children) {
                if (!argument) {
                    return nullptr;
                }
            }
            expression = std::move(call);
        } else if (match_lexeme("++") || match_lexeme("--")) {
            const Token operation = previous();
            AstNodePtr node = make_ast_node(AstKind::PostfixExpression,
                                            expression->location, operation.lexeme);
            node->children.push_back(std::move(expression));
            expression = std::move(node);
        } else {
            break;
        }
    }
    return expression;
}

AstNodePtr Parser::parse_primary() {
    if (match_lexeme("true") || match_lexeme("false")) {
        const Token& literal = previous();
        return make_ast_node(AstKind::BooleanLiteral, literal.location, literal.lexeme);
    }
    if (check(TokenType::Identifier)) {
        const Token& name = advance();
        return make_ast_node(AstKind::NameExpression, name.location, name.lexeme);
    }
    if (check(TokenType::IntegerLiteral)) {
        const Token& literal = advance();
        return make_ast_node(AstKind::IntegerLiteral, literal.location, literal.lexeme);
    }
    if (check(TokenType::StringLiteral)) {
        const Token& literal = advance();
        return make_ast_node(AstKind::StringLiteral, literal.location, literal.lexeme);
    }
    if (check(TokenType::CharacterLiteral)) {
        const Token& literal = advance();
        return make_ast_node(AstKind::CharacterLiteral, literal.location, literal.lexeme);
    }
    if (match_lexeme("(")) {
        AstNodePtr expression = parse_assignment();
        if (!match_lexeme(")")) {
            report("Expected ')' after expression.");
            return nullptr;
        }
        return expression;
    }

    report("Expected an expression.");
    if (!at_end()) {
        advance();
    }
    return nullptr;
}

}
