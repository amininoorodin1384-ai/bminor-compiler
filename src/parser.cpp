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

const Token& Parser::peek(std::size_t distance) const {
    const std::size_t position = index_ + distance;
    return position < tokens_.size() ? tokens_[position] : tokens_.back();
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

bool Parser::expect_lexeme(const std::string& lexeme, const std::string& message) {
    if (match_lexeme(lexeme)) {
        return true;
    }
    report(message);
    return false;
}

bool Parser::looks_like_variable_declaration() const {
    return check(TokenType::Identifier) && peek(1).lexeme == ":";
}

void Parser::synchronize_statement() {
    while (!at_end()) {
        if (previous().lexeme == ";") {
            return;
        }
        if (check_lexeme("}") || check_lexeme("if") || check_lexeme("return")) {
            return;
        }
        advance();
    }
}

void Parser::synchronize_declaration() {
    while (!at_end()) {
        if (previous().lexeme == ";") {
            return;
        }
        if (looks_like_variable_declaration()) {
            return;
        }
        advance();
    }
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

AstNodePtr Parser::parse_program() {
    const SourceLocation location = current().location;
    AstNodePtr program = make_ast_node(AstKind::Program, location);

    while (!at_end()) {
        const std::size_t start = index_;
        if (looks_like_variable_declaration()) {
            AstNodePtr declaration = parse_declaration();
            if (declaration) {
                program->children.push_back(std::move(declaration));
            } else {
                synchronize_declaration();
            }
        } else {
            report("Expected a top-level declaration.");
            synchronize_declaration();
        }
        if (index_ == start && !at_end()) {
            advance();
        }
    }

    return program;
}

AstNodePtr Parser::parse_declaration() {
    const Token name = advance();
    if (!expect_lexeme(":", "Expected ':' after declaration name.")) {
        return nullptr;
    }

    if (match_lexeme("function")) {
        return parse_function_declaration(name);
    }

    AstNodePtr type = parse_type(true);
    if (!type) {
        return nullptr;
    }

    AstNodePtr declaration = make_ast_node(AstKind::VariableDeclaration,
                                           name.location, name.lexeme);
    declaration->children.push_back(std::move(type));
    if (match_lexeme("=")) {
        AstNodePtr initializer = parse_initializer();
        if (!initializer) {
            return nullptr;
        }
        declaration->children.push_back(std::move(initializer));
    }
    if (!expect_lexeme(";", "Expected ';' after variable declaration.")) {
        return nullptr;
    }
    return declaration;
}

AstNodePtr Parser::parse_variable_declaration() {
    const Token name = advance();
    if (!expect_lexeme(":", "Expected ':' after local variable name.")) {
        return nullptr;
    }

    AstNodePtr type = parse_type(true);
    if (!type) {
        return nullptr;
    }

    AstNodePtr declaration = make_ast_node(AstKind::VariableDeclaration,
                                           name.location, name.lexeme);
    declaration->children.push_back(std::move(type));
    if (match_lexeme("=")) {
        AstNodePtr initializer = parse_initializer();
        if (!initializer) {
            return nullptr;
        }
        declaration->children.push_back(std::move(initializer));
    }
    if (!expect_lexeme(";", "Expected ';' after local variable declaration.")) {
        return nullptr;
    }
    return declaration;
}

AstNodePtr Parser::parse_function_declaration(const Token& name) {
    AstNodePtr return_type;
    if (match_lexeme("void")) {
        const Token& token = previous();
        return_type = make_ast_node(AstKind::AtomicType, token.location, token.lexeme);
    } else {
        return_type = parse_atomic_type();
    }
    if (!return_type) {
        return nullptr;
    }
    if (!expect_lexeme("(", "Expected '(' after function return type.")) {
        return nullptr;
    }

    std::vector<AstNodePtr> parameters;
    if (!check_lexeme(")")) {
        do {
            if (!check(TokenType::Identifier)) {
                report("Expected a parameter name.");
                synchronize_statement();
                return nullptr;
            }
            const Token parameter_name = advance();
            if (!expect_lexeme(":", "Expected ':' after parameter name.")) {
                return nullptr;
            }
            AstNodePtr parameter_type = parse_type(false);
            if (!parameter_type) {
                return nullptr;
            }
            AstNodePtr parameter = make_ast_node(AstKind::Parameter,
                                                 parameter_name.location,
                                                 parameter_name.lexeme);
            parameter->children.push_back(std::move(parameter_type));
            parameters.push_back(std::move(parameter));
        } while (match_lexeme(","));
    }

    if (!expect_lexeme(")", "Expected ')' after function parameters.")) {
        return nullptr;
    }

    if (match_lexeme(";")) {
        AstNodePtr prototype = make_ast_node(AstKind::FunctionPrototype,
                                             name.location, name.lexeme);
        prototype->children.push_back(std::move(return_type));
        for (AstNodePtr& parameter : parameters) {
            prototype->children.push_back(std::move(parameter));
        }
        return prototype;
    }

    if (!expect_lexeme("=", "Expected ';' or '= block' after function signature.")) {
        return nullptr;
    }
    AstNodePtr body = parse_block();
    if (!body) {
        return nullptr;
    }

    AstNodePtr function = make_ast_node(AstKind::FunctionDeclaration,
                                        name.location, name.lexeme);
    function->children.push_back(std::move(return_type));
    for (AstNodePtr& parameter : parameters) {
        function->children.push_back(std::move(parameter));
    }
    function->children.push_back(std::move(body));
    return function;
}

AstNodePtr Parser::parse_type(bool allow_array_size) {
    if (!match_lexeme("array")) {
        return parse_atomic_type();
    }

    const Token array_token = previous();
    if (!expect_lexeme("[", "Expected '[' after array type.")) {
        return nullptr;
    }

    std::string size;
    if (allow_array_size) {
        if (!check(TokenType::IntegerLiteral)) {
            report("Expected a fixed integer size for array type.");
            return nullptr;
        }
        size = advance().lexeme;
    }

    if (!expect_lexeme("]", "Expected ']' after array size.")) {
        return nullptr;
    }
    AstNodePtr element_type = parse_atomic_type();
    if (!element_type) {
        return nullptr;
    }

    AstNodePtr type = make_ast_node(AstKind::ArrayType, array_token.location, size);
    type->children.push_back(std::move(element_type));
    return type;
}

AstNodePtr Parser::parse_atomic_type() {
    if (check_lexeme("integer") || check_lexeme("boolean") ||
        check_lexeme("char") || check_lexeme("string")) {
        const Token type_token = advance();
        return make_ast_node(AstKind::AtomicType, type_token.location, type_token.lexeme);
    }
    report("Expected an atomic type.");
    return nullptr;
}

AstNodePtr Parser::parse_initializer() {
    if (!match_lexeme("{")) {
        return parse_assignment();
    }

    const Token opening_brace = previous();
    AstNodePtr initializer = make_ast_node(AstKind::ArrayInitializer,
                                           opening_brace.location);
    if (check_lexeme("}")) {
        report("Array initializer must contain at least one expression.");
        return nullptr;
    }

    initializer->children.push_back(parse_assignment());
    while (match_lexeme(",")) {
        initializer->children.push_back(parse_assignment());
    }
    for (const AstNodePtr& element : initializer->children) {
        if (!element) {
            return nullptr;
        }
    }
    if (!expect_lexeme("}", "Expected '}' after array initializer.")) {
        return nullptr;
    }
    return initializer;
}

AstNodePtr Parser::parse_block() {
    if (!expect_lexeme("{", "Expected '{' to begin block.")) {
        return nullptr;
    }

    const Token opening_brace = previous();
    AstNodePtr block = make_ast_node(AstKind::BlockStatement, opening_brace.location);
    while (!at_end() && !check_lexeme("}")) {
        const std::size_t start = index_;
        AstNodePtr item = looks_like_variable_declaration()
            ? parse_variable_declaration()
            : parse_statement();
        if (item) {
            block->children.push_back(std::move(item));
        } else {
            synchronize_statement();
        }
        if (index_ == start && !at_end() && !check_lexeme("}")) {
            advance();
        }
    }

    if (!expect_lexeme("}", "Expected '}' after block.")) {
        return nullptr;
    }
    return block;
}

AstNodePtr Parser::parse_statement() {
    if (check_lexeme("{")) {
        return parse_block();
    }
    if (match_lexeme("if")) {
        return parse_if_statement();
    }
    if (match_lexeme("for")) {
        return parse_for_statement();
    }
    if (match_lexeme("print")) {
        return parse_print_statement();
    }
    if (match_lexeme("return")) {
        return parse_return_statement();
    }
    if (check(TokenType::Invalid)) {
        report("Invalid token in statement.");
        advance();
        return nullptr;
    }

    const SourceLocation location = current().location;
    AstNodePtr expression = parse_assignment();
    if (!expression) {
        return nullptr;
    }
    if (!expect_lexeme(";", "Expected ';' after expression statement.")) {
        return nullptr;
    }
    AstNodePtr statement = make_ast_node(AstKind::ExpressionStatement, location);
    statement->children.push_back(std::move(expression));
    return statement;
}

AstNodePtr Parser::parse_if_statement() {
    const Token keyword = previous();
    if (!expect_lexeme("(", "Expected '(' after if.")) {
        return nullptr;
    }
    AstNodePtr condition = parse_assignment();
    if (!expect_lexeme(")", "Expected ')' after if condition.")) {
        return nullptr;
    }
    AstNodePtr then_branch = parse_statement();
    if (!condition || !then_branch) {
        return nullptr;
    }

    AstNodePtr statement = make_ast_node(AstKind::IfStatement, keyword.location);
    statement->children.push_back(std::move(condition));
    statement->children.push_back(std::move(then_branch));
    if (match_lexeme("else")) {
        AstNodePtr else_branch = parse_statement();
        if (!else_branch) {
            return nullptr;
        }
        statement->children.push_back(std::move(else_branch));
    }
    return statement;
}

AstNodePtr Parser::parse_for_statement() {
    const Token keyword = previous();
    if (!expect_lexeme("(", "Expected '(' after for.")) {
        return nullptr;
    }

    AstNodePtr initializer = check_lexeme(";")
        ? make_ast_node(AstKind::Empty, current().location)
        : parse_assignment();
    if (!expect_lexeme(";", "Expected ';' after for initializer.")) {
        return nullptr;
    }

    AstNodePtr condition = check_lexeme(";")
        ? make_ast_node(AstKind::Empty, current().location)
        : parse_assignment();
    if (!expect_lexeme(";", "Expected ';' after for condition.")) {
        return nullptr;
    }

    AstNodePtr update = check_lexeme(")")
        ? make_ast_node(AstKind::Empty, current().location)
        : parse_assignment();
    if (!expect_lexeme(")", "Expected ')' after for clauses.")) {
        return nullptr;
    }

    AstNodePtr body = parse_statement();
    if (!initializer || !condition || !update || !body) {
        return nullptr;
    }

    AstNodePtr statement = make_ast_node(AstKind::ForStatement, keyword.location);
    statement->children.push_back(std::move(initializer));
    statement->children.push_back(std::move(condition));
    statement->children.push_back(std::move(update));
    statement->children.push_back(std::move(body));
    return statement;
}

AstNodePtr Parser::parse_print_statement() {
    const Token keyword = previous();
    AstNodePtr statement = make_ast_node(AstKind::PrintStatement, keyword.location);
    statement->children.push_back(parse_assignment());
    while (match_lexeme(",")) {
        statement->children.push_back(parse_assignment());
    }
    for (const AstNodePtr& expression : statement->children) {
        if (!expression) {
            return nullptr;
        }
    }
    if (!expect_lexeme(";", "Expected ';' after print statement.")) {
        return nullptr;
    }
    return statement;
}

AstNodePtr Parser::parse_return_statement() {
    const Token keyword = previous();
    AstNodePtr statement = make_ast_node(AstKind::ReturnStatement, keyword.location);
    if (!check_lexeme(";")) {
        AstNodePtr expression = parse_assignment();
        if (!expression) {
            return nullptr;
        }
        statement->children.push_back(std::move(expression));
    }
    if (!expect_lexeme(";", "Expected ';' after return statement.")) {
        return nullptr;
    }
    return statement;
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
