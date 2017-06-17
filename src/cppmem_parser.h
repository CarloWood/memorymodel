#pragma once

#include <string>

namespace ast {
struct cppmem;
}

template<typename Iterator> struct position_handler;

namespace cppmem {

using iterator_type = std::string::const_iterator;
bool parse(iterator_type& begin, iterator_type const& end, position_handler<iterator_type>& handler, ast::cppmem& out);

} // namespace cppmem
