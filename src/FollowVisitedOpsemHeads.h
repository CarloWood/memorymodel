#pragma once
#include "Edge.h"

struct FollowVisitedOpsemHeads
{
 private:
  int m_visited_generation;

 public:
  FollowVisitedOpsemHeads(int visited_generation) : m_visited_generation(visited_generation) { }

  bool operator()(EndPoint const& end_point) const
  {
    if (end_point.type() != head || end_point.edge_type() != edge_asw)
      return end_point.type() == head && end_point.edge_type() == edge_sb;
    end_point.edge()->visited(m_visited_generation);
    return end_point.other_node()->is_fully_visited(m_visited_generation);
  }
};
