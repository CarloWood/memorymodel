#pragma once

#include "ast.h"
#include "utils/Singleton.h"
#include <stack>
#include <map>
#include "boost/spirit/include/qi_symbols.hpp"

namespace parser {

namespace qi = boost::spirit::qi;

class Symbols : public Singleton<Symbols> {
  friend_Instance;
 private:
  Symbols() = default;
  ~Symbols() = default;
  Symbols(Symbols const&) = delete;

  std::stack<std::map<int, std::string>> m_na_symbol_stack;
  std::stack<std::map<int, std::string>> m_symbol_stack;
  std::stack<std::map<int, std::string>> m_register_stack;

 public:
  qi::symbols<char, int> function_names;
  std::map<int, std::string> function_names_map;
  qi::symbols<char, int> na_memory_locations;
  std::map<int, std::string> na_memory_locations_map;
  qi::symbols<char, int> atomic_memory_locations;
  std::map<int, std::string> atomic_memory_locations_map;
  qi::symbols<char, int> register_locations;
  std::map<int, std::string> register_locations_map;
  std::map<int, std::string> all_symbols;

  void function(ast::function const& name);
  void vardecl(ast::memory_location const& memory_location);
  void regdecl(ast::register_location const& register_location);
  int set_register_id(ast::register_location const& register_location);
  void scope(int begin);
  std::string id_to_string(int id);
  void print(boost::spirit::qi::symbols<char, int>&) const;
  void reset();
};

} // namespace parser
