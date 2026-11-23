#include "bminor/lexer.hpp"
#include "bminor/parser.hpp"

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

bminor::AstNodePtr parse_expression(const std::string& source,
                                   std::vector<bminor::ParseError>& errors) {
    bminor::Lexer lexer(source, "expression.bm");
    bminor::Parser parser(lexer.scan_all());
    bminor::AstNodePtr expression = parser.parse_expression();
    errors = parser.errors();
    return expression;
}

void test_operator_precedence() {
    std::vector<bminor::ParseError> errors;
    bminor::AstNodePtr expression = parse_expression("x + 2 * 3", errors);

    require(expression != nullptr, "precedence expression has AST");
    require(errors.empty(), "precedence expression has no errors");
    require(expression->kind == bminor::AstKind::BinaryExpression, "addition is root");
    require(expression->value == "+", "root operator is addition");
    require(expression->children[1]->value == "*", "multiplication binds more tightly");
}

void test_assignment_and_exponent_associativity() {
    std::vector<bminor::ParseError> errors;
    bminor::AstNodePtr assignment = parse_expression("a = b = 3", errors);

    require(assignment != nullptr, "assignment expression has AST");
    require(errors.empty(), "assignment expression has no errors");
    require(assignment->kind == bminor::AstKind::AssignmentExpression,
            "assignment node kind");
    require(assignment->children[1]->kind == bminor::AstKind::AssignmentExpression,
            "assignment is right-associative");

    bminor::AstNodePtr exponent = parse_expression("2 ^ 3 ^ 4", errors);
    require(exponent != nullptr, "exponent expression has AST");
    require(errors.empty(), "exponent expression has no errors");
    require(exponent->children[1]->value == "^", "exponent is right-associative");
}

void test_postfix_call_and_index() {
    std::vector<bminor::ParseError> errors;
    bminor::AstNodePtr expression = parse_expression("f(1, 2)[i]++", errors);

    require(expression != nullptr, "postfix expression has AST");
    require(errors.empty(), "postfix expression has no errors");
    require(expression->kind == bminor::AstKind::PostfixExpression, "postfix node is root");
    require(expression->value == "++", "postfix operator is retained");
    require(expression->children[0]->kind == bminor::AstKind::IndexExpression,
            "array indexing is represented");
    require(expression->children[0]->children[0]->kind == bminor::AstKind::CallExpression,
            "function call is represented before indexing");
}

void test_logical_precedence_and_parse_error() {
    std::vector<bminor::ParseError> errors;
    bminor::AstNodePtr expression = parse_expression("a < b && c == d || false", errors);

    require(expression != nullptr, "logical expression has AST");
    require(errors.empty(), "logical expression has no errors");
    require(expression->value == "||", "logical OR is the root");
    require(expression->children[0]->value == "&&", "logical AND binds more tightly than OR");

    bminor::AstNodePtr malformed = parse_expression("1 +", errors);
    require(malformed == nullptr, "incomplete expression does not produce a full AST");
    require(!errors.empty(), "incomplete expression reports an error");
    require(errors[0].location.filename == "expression.bm", "parse error has source filename");
}

}

int main() {
    test_operator_precedence();
    test_assignment_and_exponent_associativity();
    test_postfix_call_and_index();
    test_logical_precedence_and_parse_error();
    std::cout << "Parser expression tests passed.\n";
    return 0;
}
