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
  os << '"' << node.name() << ':' << node.type() << ' ' << node.m_variable << '=' << node.m_value << '"';
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
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable)));
}

void Graph::write(ast::tag variable, Context& context)
{
  DoutTag(dc::notice, "[NA write to", variable);
  Value v;
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, v)));
}

void Graph::read(ast::tag variable, std::memory_order mo, Context& context)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, mo)));
}

void Graph::write(ast::tag variable, std::memory_order mo, Context& context)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  Value v;
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, variable, v, mo)));
}

void Graph::lockdecl(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_decl)));
}

void Graph::lock(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[lock of", mutex);
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_lock1)));
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_lock2)));
}

void Graph::unlock(ast::tag mutex, Context& context)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_unlock1)));
  new_node(m_nodes.insert(m_nodes.end(), Node(m_next_node_id, m_current_thread, mutex, mutex_unlock2)));
}

void Graph::new_node(nodes_type::iterator const& node)
{
  DebugMarkUp;
  Dout(dc::notice, "Created node " << *node << '.');
}

void Graph::scope_start(bool is_thread)
{
  DoutEntering(dc::sb_barrier, "Graph::scope_start('is_thread' = " << is_thread << ")");
  m_threads.push(is_thread);
  if (is_thread)
  {
    m_beginning_of_thread = true;
    m_current_thread = Thread::create_new_thread(m_next_thread_id, m_current_thread);
    DebugMarkDown;
    Dout(dc::notice, "Created " << m_current_thread << '.');
  }
}

void Graph::scope_end()
{
  bool is_thread = m_threads.top();
  DoutEntering(dc::sb_barrier, "Graph::scope_end() [is_thread = " << is_thread << "].");
  m_threads.pop();
  if (is_thread)
  {
    DebugMarkUp;
    Dout(dc::notice, "Joined thread " << m_current_thread << '.');
    m_current_thread = m_current_thread->parent_thread();
  }
}

char const* edge_str(edge_type type)
{
  switch (type)
  {
    case edge_sb: return "Sequenced-Before";
    case edge_asw: return "Additional-Synchronises-With";
    case edge_dd: return "Data-Dependency";
    case edge_cd: return "Control-Dependency";
    AI_CASE_RETURN(edge_rf);
    AI_CASE_RETURN(edge_tot);
    AI_CASE_RETURN(edge_mo);
    AI_CASE_RETURN(edge_sc);
    AI_CASE_RETURN(edge_lo);
    AI_CASE_RETURN(edge_hb);
    AI_CASE_RETURN(edge_vse);
    AI_CASE_RETURN(edge_vsses);
    AI_CASE_RETURN(edge_ithb);
    AI_CASE_RETURN(edge_dob);
    AI_CASE_RETURN(edge_cad);
    AI_CASE_RETURN(edge_sw);
    AI_CASE_RETURN(edge_hrs);
    AI_CASE_RETURN(edge_rs);
    AI_CASE_RETURN(edge_data_races);
    AI_CASE_RETURN(edge_unsequenced_races);
  }
  return "UNKNOWN edge_type";
}

std::ostream& operator<<(std::ostream& os, edge_type type)
{
  return os << edge_str(type);
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_barrier("SBBARRIER");
channel_ct edge("EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
