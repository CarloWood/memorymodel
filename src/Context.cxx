#include "sys.h"
#include "Context.h"
#include "Graph.h"

void Context::uninitialized(ast::tag decl)
{
  m_graph.uninitialized(decl, *this);
}

void Context::read(ast::tag variable)
{
  m_graph.read(variable, *this);
}

void Context::write(ast::tag variable)
{
  m_graph.write(variable, *this);
}

void Context::read(ast::tag variable, std::memory_order mo)
{
  m_graph.read(variable, mo, *this);
}

void Context::write(ast::tag variable, std::memory_order mo)
{
  m_graph.write(variable, mo, *this);
}

void Context::lockdecl(ast::tag mutex)
{
  m_graph.lockdecl(mutex, *this);
}

void Context::lock(ast::tag mutex)
{
  m_graph.lock(mutex, *this);
}

void Context::unlock(ast::tag mutex)
{
  m_graph.unlock(mutex, *this);
}
