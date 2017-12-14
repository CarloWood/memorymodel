#pragma once

#include "DirectedEdgeTails.h"
#include <vector>

class Graph;
class DirectedEdgeTails;
class MultiLoop;
class ReadFromLocationSubgraphs;

class DirectedSubgraph
{
 public:
  using nodes_type = utils::Vector<DirectedEdgeTails, TopologicalOrderedActionsIndex>;

 protected:
  nodes_type m_nodes;
  boolean::Expression m_condition;              // The condition under which this subgraph is valid.

 public:
  DirectedSubgraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition);

  // Add this subgraph to graph.
  void add_to(Graph& graph) const;
  // Return condition under which this subgraph is valid.
  boolean::Expression const& valid() const { return m_condition; }
  // Return the outgoing edges of node n for this subgraph.
  DirectedEdgeTails const& tails(TopologicalOrderedActionsIndex n) const { return m_nodes[n]; }

#if 0
  nodes_type::iterator begin() { return m_nodes.begin(); }
  nodes_type::const_iterator begin() const { return m_nodes.begin(); }
  nodes_type::const_iterator end() const { return m_nodes.end(); }
#endif

  friend std::ostream& operator<<(std::ostream& os, DirectedSubgraph const& directed_subgraph);
};
