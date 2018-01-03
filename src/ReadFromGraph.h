#pragma once

#include "Properties.h"
#include "ReadFromLocationSubgraphs.h"
#include "TopologicalOrderedActions.h"
#include "RFLocationOrderedSubgraphs.h"
#include "ast_tag.h"

class ReadFromGraph : public DirectedSubgraph
{
 private:
  using set_type = uint32_t;

  struct NodeData {
    set_type m_set;            // The set type (unvisited, followed, visited, processed) of each node.
    Properties m_properties;   // The Property objects collected so far for this node.
    NodeData() : m_set(0) { }
  };

  SequenceNumber m_current_node;                                // The current node in the Depth-First-Search.
  int const m_number_of_nodes;                                  // Copy of DirectedSubgraph::m_nodes.size().
  set_type m_generation;                                        // The current generation.
  boolean::Expression m_loop_condition;                         // Collector for the total condition under which there is any loop.
  RFLocationOrderedSubgraphs m_current_subgraphs;               // List of subgraphs that make up the current graph.
  utils::Vector<NodeData, SequenceNumber> m_node_data;          // The node data, using the nodes id as index.
  TopologicalOrderedActions const& m_topological_ordered_actions;    // Maps node sequence numbers to Action objects.
  std::vector<RFLocation> m_location_id_to_rf_location;         // Maps location tags to an index into m_current_subgraphs.

 public:
  // Reset all nodes to the state 'unvisited'.
  void reset() { m_generation += 3; }

  // Return true if node n is not yet visited.
  bool is_unvisited(SequenceNumber n) const { return m_node_data[n].m_set <= m_generation; }

  // Mark node n as being followed (part of the current path).
  void set_followed(SequenceNumber n) { m_node_data[n].m_set = m_generation + 1; }

  // Return true we are currently in the process of following node n's children.
  bool is_followed(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 1; }

  // Mark node n as being visited (not part of the current path anymore).
  void set_visited(SequenceNumber n) { m_node_data[n].m_set = m_generation + 2; }

  // Return true if node n is already visited before.
  bool is_visited(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 2; }

  // Return true if node n was visited before but doesn't contain any relevant properties.
  bool is_visited_but_not_relevant(SequenceNumber n) const { return is_visited(n) && !m_node_data[n].m_properties.contains_relevant_property(this); }

  // Mark node n as (being part of) a fully processed chain.
  void set_processed(SequenceNumber n) { m_node_data[n].m_set = m_generation + 3; }

  // Return true if node n is already fully processed.
  bool is_processed(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 3; }

  // Constructor.
  ReadFromGraph(
      Graph const& graph,
      EdgeMaskType outgoing_type,
      EdgeMaskType incoming_type,
      TopologicalOrderedActions const& topological_ordered_actions,
      std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs_vector);

  void push(DirectedSubgraph const& directed_subgraph) { m_current_subgraphs.push_back(&directed_subgraph); }
  void pop() { m_current_subgraphs.pop_back(); }

  // Returns the condition under which a loop exists (m_loop_condition).
  boolean::Expression const& loop_detected();

  // Returns the last value calculated by loop_detected().
  boolean::Expression const& loop_condition() const { return m_loop_condition; }

  // Do a Depth-First-Search starting from node m_current_node, returning true if and only if we detected
  // at least one Property, in which case m_loop_condition is set to the (possibly zero) condition under
  // which an inconsistency was found (if any).
  // If current_memory_location is unequal zero than it is the memory location that the current node reads
  // from or writes to.
  bool dfs();

  // Return the current node in the Depth-First-Search (only valid while inside dfs()).
  SequenceNumber current_node() const { return m_current_node; }
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct readfrom;
NAMESPACE_DEBUG_CHANNELS_END
#endif
