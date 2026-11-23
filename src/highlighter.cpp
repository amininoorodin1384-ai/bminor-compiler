#include "bminor/highlighter.hpp"

#include <map>
#include <utility>

namespace bminor {
namespace {

enum class HighlightCategory {
    Plain,
    Keyword,
    Identifier,
    FunctionName,
    VariableName,
    ParameterName,
    Literal,
    Operator,
    Separator,
    Comment,
    Invalid
};

using PositionKey = std::pair<std::size_t, std::size_t>;
using PositionMap = std::map<PositionKey, std::size_t>;

PositionMap build_position_map(const std::string& source) {
    PositionMap positions;
    std::size_t line = 1;
    std::size_t column = 1;

    for (std::size_t offset = 0; offset <= source.size(); ++offset) {
        positions[PositionKey{line, column}] = offset;
        if (offset == source.size()) {
            break;
        }

        const char ch = source[offset];
        if (ch == '\r') {
            ++line;
            column = 1;
        } else if (ch == '\n') {
            if (offset == 0 || source[offset - 1] != '\r') {
                ++line;
            }
            column = 1;
        } else {
            ++column;
        }
    }
    return positions;
}

std::string escape_html(const std::string& text) {
    std::string escaped;
    for (char ch : text) {
        switch (ch) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

const char* css_class(HighlightCategory category) {
    switch (category) {
        case HighlightCategory::Plain: return "";
        case HighlightCategory::Keyword: return "kw";
        case HighlightCategory::Identifier: return "id";
        case HighlightCategory::FunctionName: return "fn";
        case HighlightCategory::VariableName: return "var";
        case HighlightCategory::ParameterName: return "param";
        case HighlightCategory::Literal: return "lit";
        case HighlightCategory::Operator: return "op";
        case HighlightCategory::Separator: return "sep";
        case HighlightCategory::Comment: return "comment";
        case HighlightCategory::Invalid: return "invalid";
    }
    return "";
}

void append_html(std::string& output, const std::string& source,
                 std::size_t begin, std::size_t end, HighlightCategory category) {
    if (begin >= end) {
        return;
    }

    const std::string text = escape_html(source.substr(begin, end - begin));
    const char* class_name = css_class(category);
    if (class_name[0] == '\0') {
        output += text;
        return;
    }
    output += "<span class=\"";
    output += class_name;
    output += "\">";
    output += text;
    output += "</span>";
}

void append_gap(std::string& output, const std::string& source,
                std::size_t begin, std::size_t end) {
    std::size_t cursor = begin;
    std::size_t index = begin;

    while (index < end) {
        if (source[index] == '/' && index + 1 < end && source[index + 1] == '/') {
            append_html(output, source, cursor, index, HighlightCategory::Plain);
            std::size_t comment_end = index + 2;
            while (comment_end < end && source[comment_end] != '\n' && source[comment_end] != '\r') {
                ++comment_end;
            }
            append_html(output, source, index, comment_end, HighlightCategory::Comment);
            cursor = comment_end;
            index = comment_end;
        } else if (source[index] == '/' && index + 1 < end && source[index + 1] == '*') {
            append_html(output, source, cursor, index, HighlightCategory::Plain);
            std::size_t comment_end = index + 2;
            while (comment_end + 1 < end &&
                   !(source[comment_end] == '*' && source[comment_end + 1] == '/')) {
                ++comment_end;
            }
            if (comment_end + 1 < end) {
                comment_end += 2;
            } else {
                comment_end = end;
            }
            append_html(output, source, index, comment_end, HighlightCategory::Comment);
            cursor = comment_end;
            index = comment_end;
        } else {
            ++index;
        }
    }

    append_html(output, source, cursor, end, HighlightCategory::Plain);
}

void collect_ast_categories(const AstNode* node, const PositionMap& positions,
                            std::map<std::size_t, HighlightCategory>& categories,
                            AstKind parent_kind, std::size_t child_index,
                            bool has_parent) {
    if (node == nullptr) {
        return;
    }

    HighlightCategory category = HighlightCategory::Plain;
    bool mark = true;
    switch (node->kind) {
        case AstKind::FunctionDeclaration:
        case AstKind::FunctionPrototype:
            category = HighlightCategory::FunctionName;
            break;
        case AstKind::VariableDeclaration:
            category = HighlightCategory::VariableName;
            break;
        case AstKind::Parameter:
            category = HighlightCategory::ParameterName;
            break;
        case AstKind::NameExpression:
            category = has_parent && parent_kind == AstKind::CallExpression && child_index == 0
                ? HighlightCategory::FunctionName
                : HighlightCategory::VariableName;
            break;
        default:
            mark = false;
            break;
    }

    if (mark) {
        const auto found = positions.find(PositionKey{node->location.line, node->location.column});
        if (found != positions.end()) {
            categories[found->second] = category;
        }
    }

    for (std::size_t i = 0; i < node->children.size(); ++i) {
        collect_ast_categories(node->children[i].get(), positions, categories,
                              node->kind, i, true);
    }
}

HighlightCategory token_category(const Token& token,
                                 const std::map<std::size_t, HighlightCategory>& ast_categories,
                                 std::size_t offset) {
    if (token.type == TokenType::Identifier) {
        const auto found = ast_categories.find(offset);
        return found == ast_categories.end() ? HighlightCategory::Identifier : found->second;
    }
    if (token.type == TokenType::Keyword) {
        if (token.lexeme == "true" || token.lexeme == "false") {
            return HighlightCategory::Literal;
        }
        return HighlightCategory::Keyword;
    }
    if (token.type == TokenType::IntegerLiteral || token.type == TokenType::StringLiteral ||
        token.type == TokenType::CharacterLiteral) {
        return HighlightCategory::Literal;
    }
    if (token.type == TokenType::Operator) {
        return HighlightCategory::Operator;
    }
    if (token.type == TokenType::Separator) {
        return HighlightCategory::Separator;
    }
    if (token.type == TokenType::Invalid) {
        return HighlightCategory::Invalid;
    }
    return HighlightCategory::Plain;
}

const char* ansi_code(HighlightCategory category) {
    switch (category) {
        case HighlightCategory::Plain: return "";
        case HighlightCategory::Keyword: return "\033[1;35m";
        case HighlightCategory::Identifier: return "\033[0;37m";
        case HighlightCategory::FunctionName: return "\033[1;34m";
        case HighlightCategory::VariableName: return "\033[0;33m";
        case HighlightCategory::ParameterName: return "\033[0;36m";
        case HighlightCategory::Literal: return "\033[0;32m";
        case HighlightCategory::Operator: return "\033[0;31m";
        case HighlightCategory::Separator: return "\033[0;37m";
        case HighlightCategory::Comment: return "\033[3;90m";
        case HighlightCategory::Invalid: return "\033[1;41;37m";
    }
    return "";
}

void append_ansi(std::string& output, const std::string& source,
                 std::size_t begin, std::size_t end, HighlightCategory category) {
    if (begin >= end) {
        return;
    }
    const char* code = ansi_code(category);
    if (code[0] == '\0') {
        output.append(source, begin, end - begin);
        return;
    }
    output += code;
    output.append(source, begin, end - begin);
    output += "\033[0m";
}

void append_ansi_gap(std::string& output, const std::string& source,
                     std::size_t begin, std::size_t end) {
    std::size_t cursor = begin;
    std::size_t index = begin;
    while (index < end) {
        if (source[index] == '/' && index + 1 < end && source[index + 1] == '/') {
            append_ansi(output, source, cursor, index, HighlightCategory::Plain);
            std::size_t comment_end = index + 2;
            while (comment_end < end && source[comment_end] != '\n' && source[comment_end] != '\r') {
                ++comment_end;
            }
            append_ansi(output, source, index, comment_end, HighlightCategory::Comment);
            cursor = comment_end;
            index = comment_end;
        } else if (source[index] == '/' && index + 1 < end && source[index + 1] == '*') {
            append_ansi(output, source, cursor, index, HighlightCategory::Plain);
            std::size_t comment_end = index + 2;
            while (comment_end + 1 < end &&
                   !(source[comment_end] == '*' && source[comment_end + 1] == '/')) {
                ++comment_end;
            }
            if (comment_end + 1 < end) {
                comment_end += 2;
            } else {
                comment_end = end;
            }
            append_ansi(output, source, index, comment_end, HighlightCategory::Comment);
            cursor = comment_end;
            index = comment_end;
        } else {
            ++index;
        }
    }
    append_ansi(output, source, cursor, end, HighlightCategory::Plain);
}

}

std::string highlight_html(const std::string& source,
                           const std::vector<Token>& tokens,
                           const AstNode* syntax_tree) {
    const PositionMap positions = build_position_map(source);
    std::map<std::size_t, HighlightCategory> ast_categories;
    collect_ast_categories(syntax_tree, positions, ast_categories,
                          AstKind::Program, 0, false);

    std::string body;
    std::size_t cursor = 0;
    for (const Token& token : tokens) {
        if (token.type == TokenType::EndOfFile) {
            break;
        }
        const auto found = positions.find(PositionKey{token.location.line, token.location.column});
        if (found == positions.end() || found->second < cursor) {
            continue;
        }
        const std::size_t start = found->second;
        append_gap(body, source, cursor, start);
        const std::size_t end = start + token.lexeme.size();
        append_html(body, source, start, end,
                    token_category(token, ast_categories, start));
        cursor = end;
    }
    append_gap(body, source, cursor, source.size());

    return "<!doctype html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "<meta charset=\"utf-8\">\n"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "<title>B-Minor syntax highlight</title>\n"
           "<style>\n"
           "body{background:#111827;color:#e5e7eb;margin:0;padding:24px;font-family:system-ui,sans-serif}\n"
           "pre{white-space:pre;overflow:auto;background:#0b1020;padding:20px;border-radius:10px;line-height:1.5}\n"
           ".kw{color:#c084fc;font-weight:700}.id{color:#e5e7eb}.fn{color:#60a5fa;font-weight:700}\n"
           ".var{color:#facc15}.param{color:#22d3ee}.lit{color:#86efac}.op{color:#fb923c}\n"
           ".sep{color:#cbd5e1}.comment{color:#94a3b8;font-style:italic}.invalid{color:#f87171;background:#450a0a}\n"
           "</style>\n"
           "</head>\n"
           "<body>\n<pre>" + body + "</pre>\n</body>\n</html>\n";
}


std::string highlight_ansi(const std::string& source,
                           const std::vector<Token>& tokens,
                           const AstNode* syntax_tree) {
    const PositionMap positions = build_position_map(source);
    std::map<std::size_t, HighlightCategory> ast_categories;
    collect_ast_categories(syntax_tree, positions, ast_categories,
                          AstKind::Program, 0, false);

    std::string output;
    std::size_t cursor = 0;
    for (const Token& token : tokens) {
        if (token.type == TokenType::EndOfFile) {
            break;
        }
        const auto found = positions.find(PositionKey{token.location.line, token.location.column});
        if (found == positions.end() || found->second < cursor) {
            continue;
        }
        const std::size_t start = found->second;
        append_ansi_gap(output, source, cursor, start);
        const std::size_t end = start + token.lexeme.size();
        append_ansi(output, source, start, end,
                    token_category(token, ast_categories, start));
        cursor = end;
    }
    append_ansi_gap(output, source, cursor, source.size());
    output += "\033[0m";
    return output;
}

}
