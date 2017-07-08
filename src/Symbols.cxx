#include "sys.h"
#include "debug.h"
#include "Symbols.h"
#include "Context.h"
#include "Graph.h"

void Symbols::add(ast::declaration_statement const& declaration_statement, ValueComputation&& initialization)
{
  m_symbols.push_back(std::make_pair(declaration_statement.name(), declaration_statement));
  m_initializations.insert(initializations_type::value_type(declaration_statement.tag(), ValueComputation::make_unique(std::move(initialization))));
}

void Symbols::scope_start(bool is_thread, Context& context)
{
  context.m_graph.scope_start(is_thread);
  Dout(dc::notice, "{");
  Debug(libcw_do.inc_indent(2));
  m_stack.push(m_symbols.size());
}

void Symbols::scope_end(Context& context)
{
  Debug(libcw_do.dec_indent(2));
  Dout(dc::notice, "}");
  context.m_graph.scope_end();
  m_symbols.resize(m_stack.top());
  m_stack.pop();
  context.m_locks.reset(m_stack.size(), context);
}

ast::declaration_statement const& Symbols::find(std::string var_name) const
{
  auto iter = m_symbols.rbegin();
  while (iter != m_symbols.rend())
  {
    if (iter->first == var_name)
      break;
    ++iter;
  }
  assert(iter != m_symbols.rend());
  return iter->second;
}
