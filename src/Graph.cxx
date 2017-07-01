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
  m_current_nodes.push_back(node);
  Dout(dc::notice, "Created node " << *node << '.'); // << "; size of m_current_nodes now " << m_current_nodes.size());
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
    // If this is ever not empty than those should be added instead!
    assert(m_current_nodes.empty());
    m_parent_thread_nodes.push(m_last_nodes);
    Dout(dc::sb_barrier, "Added m_last_nodes (size " << m_last_nodes.size() << ") to m_parent_thread_nodes. m_beginning_of_thread set true.");
    m_last_nodes.clear();
    Dout(dc::sb_barrier, "m_last_nodes cleared because entered new thread.");
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
    m_last_nodes = m_parent_thread_nodes.top();
    m_parent_thread_nodes.pop();
    Dout(dc::sb_barrier, "Popped m_last_nodes (size " << m_last_nodes.size() << ") from m_parent_thread_nodes.");
  }
}

char const* edge_str(edge_type type)
{
  switch (type)
  {
    AI_CASE_RETURN(edge_sb);
    AI_CASE_RETURN(edge_asw);
    AI_CASE_RETURN(edge_dd);
    AI_CASE_RETURN(edge_cd);
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

void Graph::add_edges(last_nodes_type const& last_nodes, last_nodes_type const& current_nodes, edge_type type)
{
  DoutEntering(dc::sb_barrier, "Graph::add_edges(..., " << type << "); [size of last_nodes = " << last_nodes.size() << ", size of current_nodes " << current_nodes.size());
  for (auto const& last_node : last_nodes)
    for (auto const& current_node : current_nodes)
    {
      m_edges.insert(edges_type::value_type(m_next_edge_id, last_node, current_node, type));
      Dout(dc::edge, "Added new " << type << " edge between " << *last_node << " and " << *current_node);
    }
}

void Graph::sequence_value_computation_barrier()
{
  DebugMarkDownRight;
  DoutEntering(dc::sb_barrier, "Graph::sequence_value_computation_barrier()");
  if (m_beginning_of_thread && !m_current_nodes.empty())
  {
    m_beginning_of_thread = false;
    last_nodes_type last_parent_nodes;
    int count = 0;
    bool found = false;
    {
#ifdef CWDEBUG
      size_t const parent_thread_nodes_size = m_parent_thread_nodes.size();
      Dout(dc::sb_barrier, "Trying to find last thread-parent nodes (size of m_parent_thread_nodes is " << parent_thread_nodes_size << ")...");
      debug::Mark marker;
#endif
      while (true)
      {
        if (m_parent_thread_nodes.empty())
        {
          Dout(dc::sb_barrier, "Nothing (left) on stack; no parent nodes found!");
          break;  // Nothing found.
        }
        last_parent_nodes = m_parent_thread_nodes.top();
        if (!last_parent_nodes.empty())
        {
          found = true;
          break;
        }
        Dout(dc::sb_barrier, "Popping empty vector from stack...");
        m_parent_thread_nodes.pop();
        ++count;
      }
      while (count--)
      {
        Dout(dc::sb_barrier, "Re-adding empty vector to stack...");
        m_parent_thread_nodes.push(last_nodes_type());
      }
      ASSERT(m_parent_thread_nodes.size() == parent_thread_nodes_size);
    }
    if (found)
    {
      add_edges(last_parent_nodes, m_current_nodes, edge_asw);
    }
  }
  if (!m_current_nodes.empty())
  {
    if (!m_last_nodes.empty())
      add_edges(m_last_nodes, m_current_nodes, edge_sb);
    m_last_nodes = m_current_nodes;
    m_current_nodes.clear();
    Dout(dc::sb_barrier, "m_last_nodes set to m_current_nodes (cleared); size: " << m_last_nodes.size());
  }
}

void Graph::sequence_barrier()
{
  sequence_value_computation_barrier();
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_barrier("SBBARRIER");
channel_ct edge("EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
