#include "sys.h"
#include "debug.h"
#include "Symbols.h"
#include "Context.h"
#include "Graph.h"

void Symbols::add(ast::declaration_statement const& declaration_statement)
{
  m_symbols.push_back(std::make_pair(declaration_statement.name(), declaration_statement));
}

void Symbols::scope_start(bool is_thread, Context& context)
{
  context.scope_start(is_thread);
  Dout(dc::notice, "{");
  Debug(libcw_do.inc_indent(2));
  m_stack.push(m_symbols.size());
}

void Symbols::scope_end(Context& context)
{
  Debug(libcw_do.dec_indent(2));
  Dout(dc::notice, "}");
  context.scope_end();
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
