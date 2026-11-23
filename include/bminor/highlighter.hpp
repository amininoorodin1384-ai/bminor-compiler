#pragma once

#include "bminor/ast.hpp"
#include "bminor/token.hpp"

#include <string>
#include <vector>

namespace bminor {

    std::string highlight_html(const std::string& source,
                               const std::vector<Token>& tokens,
                               const AstNode* syntax_tree);

    std::string highlight_ansi(const std::string& source,
                               const std::vector<Token>& tokens,
                               const AstNode* syntax_tree);

}
