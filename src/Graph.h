#pragma once

#include "Node.h"
#include "debug.h"
#include <set>
#include <stack>

class Graph
{
 public:
  using nodes_type = NodePtr::container_type;

 private:
  nodes_type m_nodes;                                   // All nodes, ordered by Node::m_id.
  NodeBase::id_type m_next_node_id;                     // The id to use for the next node.

 public:
  Graph() :
    m_next_node_id{0} { }

  void generate_dot_file(std::string const& filename, Context& context) const;

 public:
  // Add a new node.
  template<typename NODE, typename ...Args>
  NodePtr new_node(Args&&... args)
  {
    DebugMarkUp;
    NodePtr node = m_nodes.emplace_hint(m_nodes.end(), new NODE(m_next_node_id++, std::forward<Args>(args)...));
    Dout(dc::notice, "Created node " << *node << '.');
    return node;
  }

  void remove_node(NodePtr const& node)
  {
    Dout(dc::notice, "Removing Node " << *node);
    m_nodes.erase(node.get_iterator());
  }

  // Add a new edge.
  void new_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Condition const& condition = Condition());
  void new_edge(EdgeType edge_type, NodePtrConditionPair const& tail_node_ptr_condition_pair, NodePtr const& head_node)
  {
    new_edge(edge_type, tail_node_ptr_condition_pair.node(), head_node, tail_node_ptr_condition_pair.condition());
  }
};
