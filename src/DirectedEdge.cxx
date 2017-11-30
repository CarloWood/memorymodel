#include "sys.h"
#include "DirectedEdge.h"
#include "Action.h"
#include "Graph.h"
#include <iostream>

void DirectedEdge::add_to(Graph&, Action* tail_node) const
{
  tail_node->add_edge_to(m_edge_type, m_head_node, m_condition.copy());
}

std::ostream& operator<<(std::ostream& os, DirectedEdge const& directed_edge)
{
  os << "{to:" << directed_edge.m_head_node->name() <<
        "; type:" << directed_edge.m_edge_type <<
        "; condition:" << directed_edge.m_condition <<
        "; rf_not_release_acquire:" << directed_edge.m_rf_not_release_acquire << '}';
  return os;
}
