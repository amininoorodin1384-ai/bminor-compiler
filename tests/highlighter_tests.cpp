#include "bminor/highlighter.hpp"
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

void test_html_highlighter_uses_ast_and_preserves_escaping() {
    const std::string source =
        "foo: function integer (x: integer) = {\n"
        "  y: integer = x + 1; // note <tag>\n"
        "  return foo(y);\n"
        "}\n";
    bminor::Lexer lexer(source, "highlight.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    bminor::Parser parser(tokens);
    bminor::AstNodePtr program = parser.parse_program();

    require(parser.errors().empty(), "highlighter sample parses cleanly");
    const std::string html = bminor::highlight_html(source, tokens, program.get());

    require(html.find("<pre>") != std::string::npos, "HTML contains preformatted source");
    require(html.find("<style>") != std::string::npos, "HTML embeds CSS");
    require(html.find("class=\"fn\">foo</span>") != std::string::npos,
            "function declaration or call uses function style");
    require(html.find("class=\"param\">x</span>") != std::string::npos,
            "parameter name uses AST style");
    require(html.find("class=\"var\">y</span>") != std::string::npos,
            "variable declaration uses AST style");
    require(html.find("class=\"comment\">// note &lt;tag&gt;</span>") != std::string::npos,
            "comment is highlighted and HTML-escaped");
    require(html.find("class=\"lit\">1</span>") != std::string::npos,
            "integer literal is highlighted");
    require(html.find("class=\"kw\">function</span>") != std::string::npos,
            "keyword is highlighted");
}

void test_ansi_highlighter_preserves_source_and_adds_color_codes() {
    const std::string source = "value: integer = 42; // note\n";
    bminor::Lexer lexer(source, "ansi.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    const std::string output = bminor::highlight_ansi(source, tokens, nullptr);

    require(output.find("\033[") != std::string::npos,
            "ANSI output contains terminal color sequences");
    require(output.find("value") != std::string::npos,
            "ANSI output preserves identifier text");
    require(output.find("42") != std::string::npos,
            "ANSI output preserves literal text");
    require(output.find("// note") != std::string::npos,
            "ANSI output preserves comment text");
    require(output.size() > source.size(), "ANSI output adds formatting codes");
}

void test_html_highlighter_without_ast() {
    const std::string source = "integer value;";
    bminor::Lexer lexer(source, "tokens.bm");
    const std::vector<bminor::Token> tokens = lexer.scan_all();
    const std::string html = bminor::highlight_html(source, tokens, nullptr);

    require(html.find("class=\"kw\">integer</span>") != std::string::npos,
            "token-based categories work without an AST");
    require(html.find("class=\"id\">value</span>") != std::string::npos,
            "identifier has generic style without an AST");
}

}

int main() {
    test_html_highlighter_uses_ast_and_preserves_escaping();
    test_html_highlighter_without_ast();
    test_ansi_highlighter_preserves_source_and_adds_color_codes();
    std::cout << "Highlighter tests passed.\n";
    return 0;
}
