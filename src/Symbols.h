#pragma once

#include "ast.h"
#include "utils/Singleton.h"
#include <stack>
#include "boost/spirit/include/qi_symbols.hpp"

namespace parser {

namespace qi = boost::spirit::qi;

class Symbols : public Singleton<Symbols> {
  friend_Instance;
 private:
  Symbols() = default;
  ~Symbols() = default;
  Symbols(Symbols const&) = delete;

  std::stack<qi::symbols<char, int>> m_symbol_stack;
  std::stack<qi::symbols<char, int>> m_register_stack;

 public:
  qi::symbols<char, int> function_names;
  qi::symbols<char, int> memory_locations;
  qi::symbols<char, int> register_locations;

  void function(ast::function const& name);
  void vardecl(ast::memory_location const& memory_location);
  void regdecl(ast::register_location const& register_location);
  int set_register_id(ast::register_location const& register_location);
  void scope(bool begin);
  std::string id_to_string(int id);
};

} // namespace parser
