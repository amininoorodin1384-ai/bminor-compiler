#pragma once

#include "bminor/token.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace bminor {

enum class AstKind {
    Program,
    Empty,
    VariableDeclaration,
    FunctionDeclaration,
    FunctionPrototype,
    Parameter,
    AtomicType,
    ArrayType,
    BlockStatement,
    IfStatement,
    ForStatement,
    PrintStatement,
    ReturnStatement,
    ExpressionStatement,
    ArrayInitializer,
    AssignmentExpression,
    BinaryExpression,
    UnaryExpression,
    PostfixExpression,
    CallExpression,
    IndexExpression,
    NameExpression,
    IntegerLiteral,
    StringLiteral,
    CharacterLiteral,
    BooleanLiteral
};

struct AstNode {
    AstKind kind;
    SourceLocation location;
    std::string value;
    std::optional<std::string> type_annotation;
    std::vector<std::unique_ptr<AstNode>> children;

    AstNode(AstKind node_kind, SourceLocation source_location, std::string node_value = {})
        : kind(node_kind), location(std::move(source_location)), value(std::move(node_value)) {}
};

using AstNodePtr = std::unique_ptr<AstNode>;

inline AstNodePtr make_ast_node(AstKind kind, SourceLocation location, std::string value = {}) {
    return std::make_unique<AstNode>(kind, std::move(location), std::move(value));
}

inline const char* ast_kind_name(AstKind kind) {
    switch (kind) {
        case AstKind::Program: return "Program";
        case AstKind::Empty: return "Empty";
        case AstKind::VariableDeclaration: return "VariableDeclaration";
        case AstKind::FunctionDeclaration: return "FunctionDeclaration";
        case AstKind::FunctionPrototype: return "FunctionPrototype";
        case AstKind::Parameter: return "Parameter";
        case AstKind::AtomicType: return "AtomicType";
        case AstKind::ArrayType: return "ArrayType";
        case AstKind::BlockStatement: return "BlockStatement";
        case AstKind::IfStatement: return "IfStatement";
        case AstKind::ForStatement: return "ForStatement";
        case AstKind::PrintStatement: return "PrintStatement";
        case AstKind::ReturnStatement: return "ReturnStatement";
        case AstKind::ExpressionStatement: return "ExpressionStatement";
        case AstKind::ArrayInitializer: return "ArrayInitializer";
        case AstKind::AssignmentExpression: return "AssignmentExpression";
        case AstKind::BinaryExpression: return "BinaryExpression";
        case AstKind::UnaryExpression: return "UnaryExpression";
        case AstKind::PostfixExpression: return "PostfixExpression";
        case AstKind::CallExpression: return "CallExpression";
        case AstKind::IndexExpression: return "IndexExpression";
        case AstKind::NameExpression: return "NameExpression";
        case AstKind::IntegerLiteral: return "IntegerLiteral";
        case AstKind::StringLiteral: return "StringLiteral";
        case AstKind::CharacterLiteral: return "CharacterLiteral";
        case AstKind::BooleanLiteral: return "BooleanLiteral";
    }
    return "Unknown";
}

}
