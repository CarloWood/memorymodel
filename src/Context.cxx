#include "sys.h"
#include "Context.h"
#include "Graph.h"

#ifdef CWDEBUG
// To make DoutTag work.
#define context (*this)
#endif

void Context::uninitialized(ast::tag decl)
{
  DoutTag(dc::notice, "[uninitialized declaration of", decl);
}

void Context::read(ast::tag variable)
{
  DoutTag(dc::notice, "[NA read from", variable);
  m_graph.new_node(*this, false, variable);
}

void Context::write(ast::tag variable)
{
  DoutTag(dc::notice, "[NA write to", variable);
  m_graph.new_node(*this, true, variable);
}

void Context::read(ast::tag variable, std::memory_order mo)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  m_graph.new_node(*this, false, variable, mo);
}

void Context::write(ast::tag variable, std::memory_order mo)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  m_graph.new_node(*this, true, variable, mo);
}

void Context::lockdecl(ast::tag mutex)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  m_graph.new_node(*this, mutex, mutex_decl);
}

void Context::lock(ast::tag mutex)
{
  DoutTag(dc::notice, "[lock of", mutex);
  m_graph.new_node(*this, mutex, mutex_lock1);
  m_graph.new_node(*this, mutex, mutex_lock2);
}

void Context::unlock(ast::tag mutex)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  m_graph.new_node(*this, mutex, mutex_unlock1);
  m_graph.new_node(*this, mutex, mutex_unlock2);
}
