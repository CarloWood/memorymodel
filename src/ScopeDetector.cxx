#include "sys.h"
#include "debug.h"
#include "ScopeDetector.h"
#include "Context.h"

template<typename AST>
void ScopeDetector<AST>::add(AST const& ast, Context const& context)
{
  m_asts.push_back(typename m_asts_type::value_type(context.m_symbols.stack_depth(), ast));
}

template<typename AST>
void ScopeDetector<AST>::reset(size_t stack_depth, Context& context)
{
  while (!m_asts.empty() && m_asts.back().first > stack_depth)
  {
    DebugMarkUp;
    left_scope(m_asts.back().second, context);
    m_asts.erase(m_asts.end());
  }
}

template class ScopeDetector<ast::unique_lock_decl>;
