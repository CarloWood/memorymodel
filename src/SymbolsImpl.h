#pragma once

#include "ast.h"
#include "utils/Singleton.h"
#include "Symbols.h"
#include <stack>
#include <map>
#include "boost/spirit/include/qi_symbols.hpp"

namespace parser {

namespace qi = boost::spirit::qi;

class SymbolsImpl {
 private:
  friend class Symbols;
  std::stack<std::map<int, std::string>> m_na_symbol_stack;
  std::stack<std::map<int, std::string>> m_symbol_stack;
  std::stack<std::map<int, std::string>> m_register_stack;
  std::stack<std::map<int, std::string>> m_mutex_stack;
  std::stack<std::map<int, std::string>> m_condition_variable_stack;
  std::stack<std::map<int, std::string>> m_unique_lock_stack;
 public:
  qi::symbols<char, ast::tag> function_names;
  std::map<int, std::string> function_names_map;
  qi::symbols<char, ast::tag> na_memory_locations;
  std::map<int, std::string> na_memory_locations_map;
  qi::symbols<char, ast::tag> atomic_memory_locations;
  std::map<int, std::string> atomic_memory_locations_map;
  qi::symbols<char, ast::tag> register_locations;
  std::map<int, std::string> register_locations_map;
  qi::symbols<char, ast::tag> mutexes;
  std::map<int, std::string> mutexes_map;
  qi::symbols<char, ast::tag> condition_variables;
  std::map<int, std::string> condition_variables_map;
  qi::symbols<char, ast::tag> unique_locks;
  std::map<int, std::string> unique_locks_map;
  std::map<int, std::string> all_symbols;
 private:
  void print(boost::spirit::qi::symbols<char, ast::tag>&) const;
};

} // namespace parser
