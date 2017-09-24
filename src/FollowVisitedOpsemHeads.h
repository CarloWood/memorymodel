#pragma once
#include "Edge.h"

struct FollowVisitedOpsemHeads
{
 private:
  int m_visited_generation;
  boolean::Expression& m_is_fully_visited;

 public:
  FollowVisitedOpsemHeads(int visited_generation, boolean::Expression& is_fully_visited) :
      m_visited_generation(visited_generation), m_is_fully_visited(is_fully_visited) { }

  // Should we follow the edge of this end_point, that is only reached when path_condition.
  bool operator()(EndPoint const& end_point, boolean::Product const& path_condition)
  {
    // Only follow heads: we go upstream in the graph.
    if (end_point.type() != head)
      return false;
    // Is this a asw edge?
    if (end_point.edge_type() == edge_asw)
    {
      // Mark this edge as being visited under condition path_condition.
      Dout(dc::visited, "Setting " << end_point.edge_type() << " edge between " << end_point.other_node()->name() <<
          " and " << end_point.current_node()->name() << " to visited under condition " << path_condition);
      {
        DebugMarkDownRight;
        end_point.edge()->visited(m_visited_generation, path_condition);
      }
      // Only continue when all other child threads also reached the parent thread.
      {
        Dout(dc::visited|continued_cf, "Calculating under what condition node " << end_point.other_node()->name() << " is fully visited...");
        DebugMarkDownRight;
        m_is_fully_visited = end_point.other_node()->is_fully_visited(m_visited_generation, path_condition);
        Dout(dc::finish, m_is_fully_visited);
      }
      return !m_is_fully_visited.is_zero();
    }
    // We only follow sb and asw edges.
    return end_point.edge_type() == edge_sb;
  }

  boolean::Expression const& is_fully_visited() const { return m_is_fully_visited; }
};
