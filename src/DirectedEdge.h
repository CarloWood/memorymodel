#pragma once

#include "EdgeType.h"
#include "Action.h"
#include "boolean-expression/BooleanExpression.h"
#include <iosfwd>

class Graph;

class DirectedEdge
{
 private:
  Action* m_head_node;                  // The node that this edge is linked to.
  EdgeType m_edge_type;                 // The type of this edge.
  boolean::Expression m_condition;      // The condition under which this edge exists.
  bool m_rf_not_release_acquire;        // True if this is a Read-From edge that NOT release-acquire, false otherwise.

 public:
  DirectedEdge(Action* tail_node, Action* head_node, EdgeType edge_type, boolean::Expression const& condition) :
    m_head_node(head_node),
    m_edge_type(edge_type),
    m_condition(condition.copy()),
    m_rf_not_release_acquire(edge_type == edge_rf && !(
         tail_node->is_atomic() && head_node->is_atomic() &&
         (tail_node->write_memory_order() == std::memory_order_release ||
          tail_node->write_memory_order() == std::memory_order_acq_rel ||
          tail_node->write_memory_order() == std::memory_order_seq_cst) &&
         (head_node->read_memory_order() == std::memory_order_acquire ||
          head_node->read_memory_order() == std::memory_order_acq_rel ||
          head_node->read_memory_order() == std::memory_order_seq_cst))) { }

  void add_to(Graph& graph, Action* tail_node) const;
  boolean::Expression const& condition() const { return m_condition; }
  bool is_rf_not_release_acquire() const { return m_rf_not_release_acquire; }
  TopologicalOrderedActionsIndex sequence_number() const { return m_head_node->sequence_number(); }

  friend std::ostream& operator<<(std::ostream& os, DirectedEdge const& directed_edge);
};
