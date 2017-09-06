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

bool EndPoint::primary_tail(EdgeType edge_type) const
{ 
  return m_type == tail && m_other_node->first_of(head, edge_type) == m_edge;
}

bool EndPoint::primary_head(EdgeType edge_type) const
{ 
  return m_type == head && m_other_node->first_of(tail, edge_type) == m_edge;
}
