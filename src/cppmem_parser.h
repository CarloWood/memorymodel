#pragma once

#include <string>
#include "ast.h"

namespace cppmem {

void parse(std::string const& text, ast::nonterminal& out);
bool parse(char const* filename, std::string const& text, ast::cppmem& out);

} // namespace cppmem
