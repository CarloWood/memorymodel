#pragma once

#include "ast.h"
#include "utils/Singleton.h"

namespace parser {

class SymbolsImpl;

class Symbols : public Singleton<Symbols>
{
  friend_Instance;
 private:
  Symbols();
  ~Symbols();
  Symbols(Symbols const&) = delete;

 public:
  SymbolsImpl* const m_impl;

 public:
  void function(ast::function const& name);
  void vardecl(ast::memory_location const& memory_location);
  void mutex_decl(ast::mutex_decl const& mutex_decl);
  void condition_variable_decl(ast::condition_variable_decl const& condition_variable_decl);
  void unique_lock_decl(ast::unique_lock_decl const& unique_lock_decl);
  void regdecl(ast::register_location const& register_location);
  ast::tag set_register_id(ast::register_location const& register_location);
  bool is_register(ast::tag id);
  void scope(int begin);
  std::string tag_to_string(ast::tag id);
  void reset();
};

} // namespace parser
