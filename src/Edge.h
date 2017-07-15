#pragma once

#include <iosfwd>
#include <set>

class Node;

enum edge_type {
  edge_sb,               // Sequenced-Before.
  edge_asw,              // Additional-Synchronises-With.
  edge_dd,               // Data-Dependency.
  edge_cd,               // Control-Dependency.
  // Next we have several relations that are existentially quantified: for each choice of control-flow paths,
  // the program enumerates all possible alternatives of the following:
  edge_rf,               // The Reads-From relation, from writes to all the reads that read from them.
  edge_tot,              // The TOTal order of the tot model.
  edge_mo,               // The Modification Order (or coherence order) over writes to atomic locations, total per location.
  edge_sc,               // A total order over the Sequentially Consistent atomic actions.
  edge_lo,               // The Lock Order.
  // Finally there are derived relations, calculated by the model from the relations above:
  edge_hb,               // Happens-before.
  edge_vse,              // Visible side effects.
  edge_vsses,            // Visible sequences of side effects.
  edge_ithb,             // Inter-thread happens-before.
  edge_dob,              // Dependency-ordered-before.
  edge_cad,              // Carries-a-dependency-to.
  edge_sw,               // Synchronises-with.
  edge_hrs,              // Hypothetical release sequence.
  edge_rs,               // Release sequence.
  edge_data_races,       // Inter-thread data races.
  edge_unsequenced_races // Intra-thread unsequenced races, unrelated by sb.
};

std::ostream& operator<<(std::ostream& os, edge_type type);

class Edge
{
 public:
  using id_type = int;
  using nodes_type = std::set<Node>;    // Use a set because we'll have a lot of iterators pointing to Nodes.

 private:
  id_type m_id;
  nodes_type::const_iterator m_begin;
  nodes_type::const_iterator m_end;
  edge_type m_type;

 public:
  Edge(id_type& next_edge_id, nodes_type::const_iterator const& begin, nodes_type::const_iterator const& end, edge_type type) :
      m_id(next_edge_id++),
      m_begin(begin),
      m_end(end),
      m_type(type) { }

  // Accessors.
  nodes_type::const_iterator begin() const { return m_begin; }
  nodes_type::const_iterator end() const { return m_end; }
  edge_type type() const { return m_type; }

  // Less-than comparator for Graph::m_edges.
  friend bool operator<(Edge const& edge1, Edge const& edge2);
};
