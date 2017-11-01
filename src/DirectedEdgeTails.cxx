#include "sys.h"
#include "DirectedEdgeTails.h"
#include "Action.h"
#include "Edge.h"
#include <iostream>

DirectedEdgeTails::DirectedEdgeTails(EdgeMaskType edge_mask_type, Action const& action) DEBUG_ONLY(:m_action(action))
{
  for (auto&& end_point : action.get_end_points())
  {
    if ((end_point.edge_type() & edge_mask_type) && end_point.type() == tail)
    {
      m_directed_edges.emplace_back(end_point.other_node()->id(), end_point.edge()->condition());
    }
  }
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
