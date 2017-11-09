#pragma once

#include "DirectedEdge.h"
#include "EdgeType.h"
#include <vector>

class Action;
class Graph;

class DirectedEdgeTails
{
 public:
  using directed_edges_type = std::vector<DirectedEdge>;

 private:
  directed_edges_type m_directed_edges;
  Action& m_action;

 public:
  DirectedEdgeTails(EdgeMaskType edge_mask_type, Action& action);

  void add_to(Graph& graph) const;
  Action& action() const { return m_action; }

  directed_edges_type::iterator begin() { return m_directed_edges.begin(); }
  directed_edges_type::const_iterator begin() const { return m_directed_edges.begin(); }
  directed_edges_type::const_iterator end() const { return m_directed_edges.end(); }

  friend std::ostream& operator<<(std::ostream& os, DirectedEdgeTails const& directed_edge_tails);
};
