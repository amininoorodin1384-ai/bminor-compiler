#include "bminor/lexer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

    void print_usage(const std::string& program_name) {
        std::cout << "B-Minor front-end\n"
                  << "Usage:\n"
                  << "  " << program_name << " --help\n"
                  << "  " << program_name << " --tokens <source-file>\n";
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

}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    if (argc != 3 || std::string(argv[1]) != "--tokens") {
        print_usage(argv[0]);
        return 2;
    }

    const std::string filename = argv[2];
    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open source file: " << filename << '\n';
        return 1;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    bminor::Lexer lexer(buffer.str(), filename);
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    bool has_invalid_token = false;

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