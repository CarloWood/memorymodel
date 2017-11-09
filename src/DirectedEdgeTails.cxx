#include "sys.h"
#include "DirectedEdgeTails.h"
#include "Action.h"
#include "Edge.h"
#include <iostream>

DirectedEdgeTails::DirectedEdgeTails(EdgeMaskType edge_mask_type, Action& action) : m_action(action)
{
  for (auto&& end_point : action.get_end_points())
  {
    if ((end_point.edge_type() & edge_mask_type) && end_point.type() == tail)
    {
      m_directed_edges.emplace_back(end_point.other_node(), end_point.edge_type(), end_point.edge()->condition());
    }
  }
}

void DirectedEdgeTails::add_to(Graph& graph) const
{
  for (DirectedEdge const& directed_edge : m_directed_edges)
    directed_edge.add_to(graph, &m_action);
}

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, DirectedEdgeTails const& directed_edge_tails)
{
  os << directed_edge_tails.m_action.name() << ":{";
  bool first = true;
  for (auto&& directed_edge : directed_edge_tails.m_directed_edges)
  {
    if (!first)
      os << ", ";
    os << directed_edge;
    first = false;
  }
  return os << '}';
}
#endif
