#include "sys.h"
#include "DirectedEdges.h"
#include "Action.h"
#include "Edge.h"
#include <iostream>

DirectedEdges::DirectedEdges(EdgeMaskType outgoing_edge_mask_type, EdgeMaskType incoming_edge_mask_type, Action* action) : m_action(action)
{
  for (auto&& end_point : action->get_end_points())
  {
    if ((end_point.edge_type() & outgoing_edge_mask_type) && end_point.type() == tail)
      m_outgoing_edges.emplace_back(action, end_point.other_node(), end_point.edge_type(), end_point.edge()->condition());
    if ((end_point.edge_type() & incoming_edge_mask_type) && end_point.type() == head)
      m_incoming_edges.emplace_back(end_point.other_node(), action, end_point.edge_type(), end_point.edge()->condition());
  }
}

void DirectedEdges::add_to(Graph& graph) const
{
  // Don't add the "incoming" edges because they are just duplicates.
  for (DirectedEdge const& directed_edge : m_outgoing_edges)
    directed_edge.add_to(graph, m_action);
}

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, DirectedEdges const& directed_edges)
{
  os << directed_edges.m_action->name() << ":{out:";
  bool first = true;
  for (auto&& directed_edge : directed_edges.m_outgoing_edges)
  {
    if (!first)
      os << ", ";
    os << directed_edge;
    first = false;
  }
  os << "},{in:";
  first = true;
  for (auto&& directed_edge : directed_edges.m_incoming_edges)
  {
    if (!first)
      os << ", ";
    os << directed_edge;
    first = false;
  }
  return os << '}';
}
#endif
