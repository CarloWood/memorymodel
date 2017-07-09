#pragma once

#include "ast.h"
#include "TagCompare.h"
#include "Evaluation.h"
#include <vector>
#include <utility>
#include <string>
#include <map>
#include <stack>
#include <memory>

struct Context;

class Symbols {
 public:
  using symbols_type = std::vector<std::pair<std::string, ast::declaration_statement>>;
  using initializations_type = std::map<ast::tag, std::unique_ptr<Evaluation>, TagCompare>;

 private:
  symbols_type m_symbols;
  initializations_type m_initializations;
  std::stack<int> m_stack;

 public:
  void add(ast::declaration_statement const& declaration_statement, Evaluation&& initialization = Evaluation(Evaluation::not_used));
  void scope_start(bool is_thread, Context& context);
  void scope_end(Context& context);
  int stack_depth() const { return m_stack.size(); }
  ast::declaration_statement const& find(std::string var_name) const;
};
