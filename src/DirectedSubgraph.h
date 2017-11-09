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
  using nodes_type = std::vector<DirectedEdgeTails>;

 private:
  nodes_type m_nodes;
  boolean::Expression m_condition;              // The condition under which this subgraph is valid.

 public:
  DirectedSubgraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition);

  // Add this subgraph to graph.
  void add_to(Graph& graph) const;
  // Return condition under which this subgraph is valid.
  boolean::Expression const& valid() const { return m_condition; }
  // Return true if a loop is detected where this object contains the sb and asw edges,
  // and the first *ml ReadFromLocationSubgraphs's in read_from_location_subgraphs
  // contain rf edges.
  bool loop_detected(MultiLoop const& ml, std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs) const;

#if 0
  nodes_type::iterator begin() { return m_nodes.begin(); }
  nodes_type::const_iterator begin() const { return m_nodes.begin(); }
  nodes_type::const_iterator end() const { return m_nodes.end(); }
#endif

  friend std::ostream& operator<<(std::ostream& os, DirectedSubgraph const& directed_subgraph);
};
