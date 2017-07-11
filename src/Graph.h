#pragma once

#include "Node.h"
#include "debug.h"
#include <set>
#include <stack>

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
  nodes_type::iterator m_begin;
  nodes_type::iterator m_end;
  edge_type m_type;

 public:
  Edge(id_type& next_edge_id, nodes_type::iterator const& begin, nodes_type::iterator const& end, edge_type type) :
      m_id(next_edge_id++),
      m_begin(begin),
      m_end(end),
      m_type(type) { }

  // Less-than comparator for Graph::m_edges.
  friend bool operator<(Edge const& edge1, Edge const& edge2) { return edge1.m_id < edge2.m_id; }
};

class Graph
{
 public:
  using nodes_type = Edge::nodes_type;

 private:
  nodes_type m_nodes;                                    // All nodes, ordered by Node::m_id.
  Thread::id_type m_next_thread_id;                      // The id to use for the next thread.
  ThreadPtr m_current_thread;                            // The current thread.
  Node::id_type m_next_node_id;                          // The id to use for the next node.
  std::stack<bool> m_threads;                            // Whether or not current scope is a thread.
  Edge::id_type m_next_edge_id;                          // The id to use for the next edge.
  using edges_type = std::set<Edge>;                     // Use a set because we'll have a lot of iterators pointing to Edges.
  edges_type m_edges;                                    // All edges, ordered by Edge::m_id.
  bool m_beginning_of_thread;                            // Set to true when a new thread was just started.

 public:
  Graph() :
    m_next_thread_id{1},
    m_current_thread{Thread::create_main_thread()},
    m_next_node_id{0},
    m_next_edge_id{0},
    m_beginning_of_thread(false) { }

  void print_nodes() const;

 public:
  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();

  // A new node was added.
  template<typename ...Args>
  nodes_type::const_iterator new_node(Args&&... args)
  {
    DebugMarkUp;
    auto node = m_nodes.emplace_hint(m_nodes.end(), m_next_node_id++, m_current_thread, std::forward<Args>(args)...);
    Dout(dc::notice, "Created node " << *node << '.');
    return node;
  }

  void for_each_write_node(ast::tag tag, void (*f)(nodes_type::value_type const& node, Context& context), Context& context) const
  {
    for (auto&& node : m_nodes)
    {
      if (node.tag() != tag || !node.is_write())
        continue;
      f(node, context);
    }
  }
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_barrier;
extern channel_ct edge;
NAMESPACE_DEBUG_CHANNELS_END
#endif
