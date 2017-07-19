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

 public:
  Graph() :
    m_next_node_id{0} { }

  void generate_dot_file(std::string const& filename, Context& context) const;

 public:
  // Add a new node.
  template<typename ...Args>
  node_iterator new_node(Args&&... args)
  {
    DebugMarkUp;
    auto node = m_nodes.emplace_hint(m_nodes.end(), m_next_node_id++, std::forward<Args>(args)...);
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
