#pragma once

#include "DirectedEdge.h"
#include "EdgeType.h"
#include <vector>

class Action;
class Graph;

class DirectedEdges
{
 public:
  using directed_edges_type = std::vector<DirectedEdge>;

 private:
  directed_edges_type m_outgoing_edges;
  directed_edges_type m_incoming_edges;
  Action* m_action;

 public:
  DirectedEdges() { }
  DirectedEdges(EdgeMaskType outgoing_edge_mask_type, EdgeMaskType incoming_edge_mask_type, Action* action);

  void add_to(Graph& graph) const;
  Action& action() const { return *m_action; }

  directed_edges_type::iterator begin_outgoing() { return m_outgoing_edges.begin(); }
  directed_edges_type::const_iterator begin_outgoing() const { return m_outgoing_edges.begin(); }
  directed_edges_type::const_iterator end_outgoing() const { return m_outgoing_edges.end(); }

  directed_edges_type::iterator begin_incoming() { return m_incoming_edges.begin(); }
  directed_edges_type::const_iterator begin_incoming() const { return m_incoming_edges.begin(); }
  directed_edges_type::const_iterator end_incoming() const { return m_incoming_edges.end(); }

  friend std::ostream& operator<<(std::ostream& os, DirectedEdges const& directed_edges);
};
