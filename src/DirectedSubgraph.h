#pragma once

#include "DirectedEdgeTails.h"
#include <vector>

class Graph;

class DirectedSubgraph
{
 private:
  std::vector<DirectedEdgeTails> m_nodes;
  boolean::Expression m_condition;              // The condition under which this subgraph is valid.

 public:
  DirectedSubgraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition);

  friend std::ostream& operator<<(std::ostream& os, DirectedSubgraph const& directed_subgraph);
};
