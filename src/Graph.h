#pragma once

#include "Node.h"
#include "debug.h"
#include <set>
#include <stack>

class Graph
{
 public:
  using nodes_type = std::set<Node>;
  using node_iterator = nodes_type::iterator;

 private:
  nodes_type m_nodes;                                   // All nodes, ordered by Node::m_id.
  Node::id_type m_next_node_id;                         // The id to use for the next node.
  Thread::id_type m_next_thread_id;                     // The id to use for the next thread.
  ThreadPtr m_current_thread;                           // The current thread.
  std::stack<bool> m_threads;                           // Whether or not current scope is a thread.
  bool m_beginning_of_thread;                           // Set to true when a new thread was just started.

 public:
  Graph() :
    m_next_node_id{0},
    m_next_thread_id{1},
    m_current_thread{Thread::create_main_thread()},
    m_beginning_of_thread(false) { }

  void generate_dot_file(std::string const& filename) const;
  int number_of_threads() const { return m_next_thread_id; }

 public:
  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();

  // Add a new node.
  template<typename ...Args>
  node_iterator new_node(Args&&... args)
  {
    DebugMarkUp;
    auto node = m_nodes.emplace_hint(m_nodes.end(), m_next_node_id++, m_current_thread, std::forward<Args>(args)...);
    Dout(dc::notice, "Created node " << *node << '.');
    return node;
  }

  void remove_node(node_iterator const& node)
  {
    Dout(dc::notice, "Removing Node " << *node);
    m_nodes.erase(node);
  }

  // Add a new edge.
  void new_edge(EdgeType edge_type, node_iterator const& tail_node, node_iterator const& head_node);
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_edge;
NAMESPACE_DEBUG_CHANNELS_END
#endif
