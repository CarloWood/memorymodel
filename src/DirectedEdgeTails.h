#pragma once

#include "DirectedEdge.h"
#include "EdgeType.h"
#include <vector>

class Action;

class DirectedEdgeTails
{
 private:
  std::vector<DirectedEdge> m_directed_edges;
#ifdef CWDEBUG
  Action const& m_action;
#endif

 public:
  DirectedEdgeTails(EdgeMaskType edge_mask_type, Action const& action);

  friend std::ostream& operator<<(std::ostream& os, DirectedEdgeTails const& directed_edge_tails);
};
