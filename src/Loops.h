#pragma once

#include "ast.h"
#include <stack>

class Loops
{
 private:
  std::stack<ast::iteration_statement> m_stack;

 public:
  void enter(ast::iteration_statement const& iteration_statement);
  void leave(ast::iteration_statement const& iteration_statement);
  void add_break(ast::break_statement const& break_statement);
};
