#pragma once

#include "ast.h"
#include <vector>
#include <utility>
#include <cstddef>

template<typename AST>
class ScopeDetector
{
 private:
  using m_asts_type = std::vector<std::pair<size_t, AST>>;
  m_asts_type m_asts;

 public:
  void add(AST const& unique_lock_decl);
  void reset(size_t stack_depth);
  virtual void left_scope(AST const& ast) = 0;
};

extern template class ScopeDetector<ast::unique_lock_decl>;
