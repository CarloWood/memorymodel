#pragma once

#include "EdgeType.h"
#include "boolean-expression/BooleanExpression.h"
#include <iosfwd>

class Graph;
class Action;

class DirectedEdge
{
 private:
  Action* m_head_node;                  // The node that this edge is linked to.
  EdgeType m_edge_type;                 // The type of this edge.
  boolean::Expression m_condition;      // The condition under which this edge exists.

 public:
  DirectedEdge(Action* head_node, EdgeType edge_type, boolean::Expression const& condition) :
    m_head_node(head_node), m_edge_type(edge_type), m_condition(condition.copy()) { }

  void add_to(Graph& graph, Action* tail_node) const;

  friend std::ostream& operator<<(std::ostream& os, DirectedEdge const& directed_edge);
};
