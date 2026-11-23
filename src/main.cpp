#include "bminor/highlighter.hpp"
#include "bminor/lexer.hpp"
#include "bminor/parser.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

void print_usage(const std::string& program_name) {
    std::cout << "B-Minor front-end\n"
              << "Usage:\n"
              << "  " << program_name << " --help\n"
              << "  " << program_name << " --tokens <source-file>\n"
              << "  " << program_name << " --parse <source-file>\n"
              << "  " << program_name << " --highlight-ansi <source-file>\n"
              << "  " << program_name << " --highlight-html <source-file> [output.html]\n";
}

std::string escape_lexeme(const std::string& text) {
    std::string escaped;
    for (char ch : text) {
        switch (ch) {
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

bool read_source_file(const std::string& filename, std::string& source) {
    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open source file: " << filename << '\n';
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    source = buffer.str();
    return true;
}

void print_ast(const bminor::AstNode& node, std::size_t depth) {
    std::cout << std::string(depth * 2, ' ') << bminor::ast_kind_name(node.kind);
    if (!node.value.empty()) {
        std::cout << " \"" << escape_lexeme(node.value) << "\"";
    }
    if (node.type_annotation.has_value()) {
        std::cout << " type=" << *node.type_annotation;
    }
    std::cout << '\n';

    for (const std::unique_ptr<bminor::AstNode>& child : node.children) {
        if (child) {
            print_ast(*child, depth + 1);
        }
    }
}

}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return 2;
    }

    const std::string mode = argv[1];
    const bool writes_html_file = mode == "--highlight-html" && argc == 4;
    if (argc == 4 && !writes_html_file) {
        print_usage(argv[0]);
        return 2;
    }
    const std::string filename = argv[2];
    std::string source;
    if (!read_source_file(filename, source)) {
        return 1;
    }

    bminor::Lexer lexer(source, filename);
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    bool has_invalid_token = false;

    if (mode == "--tokens") {
        for (const bminor::Token& token : tokens) {
            std::cout << token.location.filename << ':'
                      << token.location.line << ':'
                      << token.location.column << ' '
                      << bminor::token_type_name(token.type) << " \""
                      << escape_lexeme(token.lexeme) << "\"\n";
            if (token.type == bminor::TokenType::Invalid) {
                has_invalid_token = true;
            }
        }
        return has_invalid_token ? 1 : 0;
    }

    if (mode == "--highlight-ansi" || mode == "--highlight-html") {
        bminor::Parser parser(tokens);
        const bminor::AstNodePtr program = parser.parse_program();
        for (const bminor::Token& token : tokens) {
            if (token.type == bminor::TokenType::Invalid) {
                std::cerr << token.location.filename << ':'
                          << token.location.line << ':'
                          << token.location.column
                          << ": error: invalid token \""
                          << escape_lexeme(token.lexeme) << "\"\n";
                has_invalid_token = true;
            }
        }
        for (const bminor::ParseError& error : parser.errors()) {
            std::cerr << error.location.filename << ':'
                      << error.location.line << ':'
                      << error.location.column
                      << ": error: " << error.message << '\n';
        }

        const std::string highlighted = mode == "--highlight-html"
            ? bminor::highlight_html(source, tokens, program.get())
            : bminor::highlight_ansi(source, tokens, program.get());
        if (writes_html_file) {
            const std::string output_filename = argv[3];
            std::ofstream output(output_filename, std::ios::binary);
            if (!output) {
                std::cerr << "Cannot create HTML output file: " << output_filename << '\n';
                return 1;
            }
            output << highlighted;
            if (!output) {
                std::cerr << "Failed while writing HTML output file: " << output_filename << '\n';
                return 1;
            }
        } else {
            std::cout << highlighted;
        }
        return has_invalid_token || !parser.errors().empty() ? 1 : 0;
    }

    if (mode == "--parse") {
        for (const bminor::Token& token : tokens) {
            if (token.type == bminor::TokenType::Invalid) {
                std::cerr << token.location.filename << ':'
                          << token.location.line << ':'
                          << token.location.column
                          << ": error: invalid token \""
                          << escape_lexeme(token.lexeme) << "\"\n";
                has_invalid_token = true;
            }
        }

        bminor::Parser parser(tokens);
        const bminor::AstNodePtr program = parser.parse_program();
        for (const bminor::ParseError& error : parser.errors()) {
            std::cerr << error.location.filename << ':'
                      << error.location.line << ':'
                      << error.location.column
                      << ": error: " << error.message << '\n';
        }
        if (program) {
            print_ast(*program, 0);
        }
        return has_invalid_token || !parser.errors().empty() ? 1 : 0;
    }

    print_usage(argv[0]);
    return 2;
}
