#pragma once

#include <string>
#include "cppmem.h"

namespace cppmem
{

void parse(std::string const& text, AST::nonterminal& out);
bool parse(std::string const& text, AST::cppmem& out);

} // namespace cppmem
