#pragma once

#include "Node.h"
#include "debug.h"
#include "NodePtrConditionPair.h"
#include "Action.inl"   // Action::for_actions.
#include <memory>
#include <set>
#include <stack>

class Graph
{
 public:
  using nodes_type = NodePtr::container_type;

 private:
  nodes_type m_nodes;                   // All nodes, ordered by Node::m_id.
  Action::id_type m_next_node_id;       // The id to use for the next node.

 public:
  Graph() :
    m_next_node_id{0} { }

  template <class FOLLOW, class FILTER>
  void for_actions_no_condition(// Starting with the first Action of the program call if_found(action) when filter returns true;
      FOLLOW follow,            // If if_found() does not return true, then follow each of its EndPoints when
                                //  `bool FOLLOW::operator()(EndPoint const&)` returns true and repeat.
      FILTER filter,            // Call if_found() for this Action when `bool FILTER::operator()(Action const&)` returns true.
      std::function<bool(Action*)> const& if_found) const
  {
    if (m_nodes.empty())
      return;
    Action* action{m_nodes.begin()->get()};      // The first Action of the program.
    if (filter(*action) && if_found(action))
      return;
    action->for_actions_no_condition(follow, filter, if_found);
  }

  void generate_dot_file(std::string const& filename, std::vector<Action*> const& topological_ordered_actions, boolean::Expression const& valid) const;
  void write_png_file(std::string basename, std::vector<Action*> const& topological_ordered_actions, boolean::Expression const& valid, int appendix = -1) const;

  // Accessor.
  nodes_type::iterator begin() { return m_nodes.begin(); }
  nodes_type::const_iterator begin() const { return m_nodes.begin(); }
  nodes_type::const_iterator end() const { return m_nodes.end(); }

 public:
  // Add a new node.
  template<typename NODE, typename ...Args>
  NodePtr new_node(Args&&... args)
  {
    DebugMarkUp;
    NodePtr node{m_nodes.emplace_hint(m_nodes.end(), new NODE(m_next_node_id++, std::forward<Args>(args)...))};
    Dout(dc::nodes, "Created node " << *node << '.');
    return node;
  }

  void remove_node(NodePtr const& node)
  {
    Dout(dc::nodes, "Removing Node " << *node);
    m_nodes.erase(node.get_iterator());
  }

  // Add a new edge.
  void new_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Condition const& condition = Condition());
  void new_edge(EdgeType edge_type, NodePtrConditionPair const& tail_current_head_of_thread, NodePtr const& head_node)
  {
    new_edge(edge_type, tail_current_head_of_thread.node(), head_node, tail_current_head_of_thread.condition());
  }

  // Delete all edges of type edge_type.
  void delete_edges(EdgeType edge_type)
  {
    for (auto&& action_ptr : m_nodes)
      action_ptr->delete_endpoints(edge_type);
  }
};
