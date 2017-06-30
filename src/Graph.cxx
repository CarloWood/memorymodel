#include "sys.h"
#include "Graph.h"
#include "Context.h"
#include "debug.h"
#include "utils/macros.h"
#include <stdexcept>

std::ostream& operator<<(std::ostream& os, Thread const& thread)
{
  os << "{Thread: m_id = " << thread.m_id << "}";
  return os;
}

char const* access_str(Access access)
{
  switch (access)
  {
    AI_CASE_RETURN(ReadAccess);
    AI_CASE_RETURN(WriteAccess);
    AI_CASE_RETURN(MutexDeclaration);
    AI_CASE_RETURN(MutexLock1);
    AI_CASE_RETURN(MutexLock2);
    AI_CASE_RETURN(MutexUnlock1);
    AI_CASE_RETURN(MutexUnlock2);
  }
  throw std::runtime_error("Unknown Access type");
}

std::ostream& operator<<(std::ostream& os, Access access)
{
  return os << access_str(access);
}

std::string Node::type() const
{
  std::string type;
  switch (m_access)
  {
    case ReadAccess:
    case MutexLock1:
    case MutexUnlock1:
      type = "R";
      break;
    case WriteAccess:
    case MutexDeclaration:
      type = "W";
      break;
    case MutexLock2:
      return "LK";
    case MutexUnlock2:
      return "UL";
  }
  if (!m_atomic)
    type += "na";
  else
    switch (m_memory_order)
    {
      case std::memory_order_relaxed:   // relaxed
        type += "rlx";
        break;
      case std::memory_order_consume:   // consume
        type += "con";
        break;
      case std::memory_order_acquire:   // acquire
        type += "acq";
        break;
      case std::memory_order_release:   // release
        type += "rel";
        break;
      case std::memory_order_acq_rel:   // acquire/release
        // This should not happen because this is always either a Read or Write
        // operation and therefore either acquire or release respectively.
        DoutFatal(dc::core, "memory_order_acq_rel for Node type?!");
        break;
      case std::memory_order_seq_cst:   // sequentially consistent
        type += "sc";
        break;
    }

  return type;
}

std::ostream& operator<<(std::ostream& os, Value const& value)
{
  return os << "VALUE";
}

std::ostream& operator<<(std::ostream& os, Node const& node)
{
  os << node.name() << ':' << node.type() << ' ' << node.m_variable << '=' << node.m_value;
  return os;
}

void Graph::print_nodes() const
{
  for (auto&& node : m_nodes)
  {
    Dout(dc::notice, node);
  }
}

void Graph::uninitialized(ast::tag decl, Context& context)
{
  DoutTag(dc::notice, "[uninitialized declaration of", decl);
}

void Graph::read(ast::tag variable, Context& context)
{
  DoutTag(dc::notice, "[NA read from", variable);
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable));
}

void Graph::write(ast::tag variable, Context& context)
{
  DoutTag(dc::notice, "[NA write to", variable);
  Value v;
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, v));
}

void Graph::read(ast::tag variable, std::memory_order mo, Context& context)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, mo));
}

void Graph::write(ast::tag variable, std::memory_order mo, Context& context)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  Value v;
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, v, mo));
}

void Graph::lockdecl(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_decl));
}

void Graph::lock(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[lock of", mutex);
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_lock1));
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_lock2));
}

void Graph::unlock(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_unlock1));
  m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_unlock2));
}
