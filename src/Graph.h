#pragma once

#include "Node.h"
#include "Edge.h"
#include "debug.h"
#include <set>
#include <stack>

class Graph
{
 public:
  using nodes_type = Edge::nodes_type;

 private:
  nodes_type m_nodes;                                    // All nodes, ordered by Node::m_id.
  Thread::id_type m_next_thread_id;                      // The id to use for the next thread.
  ThreadPtr m_current_thread;                            // The current thread.
  Node::id_type m_next_node_id;                          // The id to use for the next node.
  std::stack<bool> m_threads;                            // Whether or not current scope is a thread.
  Edge::id_type m_next_edge_id;                          // The id to use for the next edge.
  using edges_type = std::set<Edge>;                     // Use a set because we'll have a lot of iterators pointing to Edges.
  edges_type m_edges;                                    // All edges, ordered by Edge::m_id.
  bool m_beginning_of_thread;                            // Set to true when a new thread was just started.

 public:
  Graph() :
    m_next_thread_id{1},
    m_current_thread{Thread::create_main_thread()},
    m_next_node_id{0},
    m_next_edge_id{0},
    m_beginning_of_thread(false) { }

  void print_nodes() const;

 public:
  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();

  // Add a new node.
  template<typename ...Args>
  nodes_type::iterator new_node(Args&&... args)
  {
    DebugMarkUp;
    auto node = m_nodes.emplace_hint(m_nodes.end(), m_next_node_id++, m_current_thread, std::forward<Args>(args)...);
    Dout(dc::notice, "Created node " << *node << '.');
    return node;
  }

  // Add a new edge.
  edges_type::iterator new_edge(nodes_type::iterator const& begin, nodes_type::iterator const& end, edge_type type)
  {
    DoutEntering(dc::notice, "new_edge(" << *begin << ", " <<  *end << ", " << type << ").");
    DebugMarkUp;
    auto edge = m_edges.emplace(m_next_edge_id, begin, end, type);
    if (edge.second)
      ++m_next_edge_id;
    return edge.first;
  }

  void for_each_write_node(ast::tag tag, void (*f)(nodes_type::value_type const& node, Context& context), Context& context) const
  {
    for (auto&& node : m_nodes)
    {
      if (node.tag() != tag || !node.is_write())
        continue;
      f(node, context);
    }
  }
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_edge;
NAMESPACE_DEBUG_CHANNELS_END
#endif
