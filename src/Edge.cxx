#include "sys.h"
#include "Edge.h"
#include "Node.h"

//  A <-- Action
//  o <-- EndPoint type tail
//  |
//  | <-- Edge
//  v
//  o <-- EndPoint type head
//  B <-- Action

bool EndPoint::primary_tail(EdgeMaskType edge_mask_type) const
{ 
  return m_type == tail && m_other_node->first_of(head, edge_mask_type) == m_edge;
}

bool EndPoint::primary_head(EdgeMaskType edge_mask_type) const
{ 
  return m_type == head && m_other_node->first_of(tail, edge_mask_type) == m_edge;
}

Action* EndPoint::current_node() const
{
  for (auto&& end_point : m_other_node->get_end_points())
    if (end_point.edge() == m_edge)
      return end_point.other_node();
  // Should always be found.
  ASSERT(false);
  return nullptr;
}
