#pragma once

#include "ast.h"
#include "Evaluation.h"
#include "utils/AIRefCount.h"
#include "utils/ulong_to_base.h"
#include <memory>
#include <utility>
#include <iosfwd>
#include <string>

class Node;
class Thread;
using ThreadPtr = boost::intrusive_ptr<Thread>;

class Thread : public AIRefCount
{
 public:
  using id_type = int;

 private:
  id_type m_id;                 // Unique identifier of a thread.
  ThreadPtr m_parent_thread;    // Only valid when m_id > 0.

 protected:
  Thread() : m_id(0) { }
  Thread(id_type id, ThreadPtr const& parent_thread) : m_id(id), m_parent_thread(parent_thread) { assert(m_id > 0); }

 public:
  static ThreadPtr create_main_thread() { return new Thread; }
  static ThreadPtr create_new_thread(id_type& next_id, ThreadPtr const& current_thread) { return new Thread(next_id++, current_thread); }
  bool is_main_thread() const { return m_id == 0; }
  id_type id() const { return m_id; }
  ThreadPtr const& parent_thread() const { assert(m_id > 0); return m_parent_thread; }

  friend bool operator==(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id == thr2->m_id; }
  friend bool operator!=(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id != thr2->m_id; }
  friend bool operator<(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id < thr2->m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread);
  friend std::ostream& operator<<(std::ostream& os, ThreadPtr const& thread) { return os << *thread; }
};

enum mutex_type
{
  mutex_decl,
  mutex_lock1,
  mutex_lock2,
  mutex_unlock1,
  mutex_unlock2
};

enum Access
{
  ReadAccess,
  WriteAccess,
  MutexDeclaration,
  MutexLock1,
  MutexLock2,
  MutexUnlock1,
  MutexUnlock2
};

inline Access convert(mutex_type mt)
{
  return static_cast<Access>(mt - mutex_lock1 + MutexLock1);
}

char const* access_str(Access access);
std::ostream& operator<<(std::ostream& os, Access access);

enum EdgeType {
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

char const* edge_str(EdgeType edge_type);
std::ostream& operator<<(std::ostream& os, EdgeType edge_type);

inline bool is_directed(EdgeType edge_type)
{
  // FIXME
  return edge_type < edge_data_races;
}

enum EndPointType {
  undirected,
  tail,                 // >>-- This end point is the tail of the arrow: the beginning of the directed edge.
  head                  // -->> This end point is the head of the arrow: the end of the directed edge.
};

char const* end_point_str(EndPointType end_point_type);
std::ostream& operator<<(std::ostream& os, EndPointType end_point_type);

class Edge
{
 private:
  EdgeType m_edge_type;

 public:
  Edge(EdgeType edge_type) : m_edge_type(edge_type) { }
  EdgeType edge_type() const { return m_edge_type; }

  friend std::ostream& operator<<(std::ostream& os, Edge const& edge);
  friend bool operator==(Edge const& edge1, Edge const& edge2) { return edge1.m_edge_type == edge2.m_edge_type; }
};

// One side of a given edge.
class EndPoint
{
 public:
  using nodes_type = std::set<Node>;            // The container type used in Graph to store Nodes.
  using node_iterator = nodes_type::iterator;   // Used as pointers to other Nodes.

 private:
  Edge* m_edge;
  EndPointType m_type;
  node_iterator m_other_node;
  bool m_edge_owner;            // Owns the allocation of m_edge.

 public:
  EndPoint(Edge* edge, EndPointType type, node_iterator const& other_node, bool edge_owner) :
      m_edge(edge), m_type(type), m_other_node(other_node), m_edge_owner(edge_owner) { }
  // Move constructor; needed because memory of a std::vector might need reallocation.
  EndPoint(EndPoint&& end_point) :
    m_edge(end_point.m_edge),
    m_type(end_point.m_type),
    m_other_node(end_point.m_other_node),
    m_edge_owner(end_point.m_edge_owner)
  {
    end_point.m_edge_owner = false;
  }
  ~EndPoint() { if (m_edge_owner) delete m_edge; }

  // Accessors.
  EdgeType edge_type() const { return m_edge->edge_type(); }
  EndPointType type() const { return m_type; }
  node_iterator other_node() const { return m_other_node; }

  friend bool operator==(EndPoint const& end_point1, EndPoint const& end_point2);
  friend std::ostream& operator<<(std::ostream& os, EndPoint const& end_point);
};

class Node
{
 public:
  using id_type = int;
  using filter_type = unsigned int;
  using end_points_type = std::vector<EndPoint>;

  // Bits
  static constexpr filter_type sequenced_before_value_computation_bit        = 1;
  static constexpr filter_type sequenced_before_side_effect_bit              = 2;
  static constexpr filter_type sequenced_after_value_computation_bit         = 4;
  static constexpr filter_type sequenced_after_side_effect_bit               = 8;
  static constexpr filter_type sequenced_before_pseudo_value_computation_bit = 16;
  static constexpr filter_type anything                                      = 32;
  static constexpr filter_type sb_unused_bit                                 = 64;

  // Masks
  static constexpr filter_type sequenced_before_mask   = sequenced_before_value_computation_bit|sequenced_before_side_effect_bit;
  static constexpr filter_type sequenced_after_mask    =  sequenced_after_value_computation_bit|sequenced_after_side_effect_bit;

  // The bits set in these masks are requested bits (they should be set).
  static constexpr filter_type side_effect_mask        =       sequenced_before_side_effect_bit|sequenced_after_side_effect_bit;
  static constexpr filter_type value_computation_mask  = sequenced_before_value_computation_bit|sequenced_after_value_computation_bit;

  // The bits set in these masks are forbidden bits (they should be unset).
  static constexpr filter_type value_computation_tails = sequenced_after_value_computation_bit;
  static constexpr filter_type value_computation_heads = sequenced_before_value_computation_bit;
  static constexpr filter_type side_effect_tails       = sequenced_after_side_effect_bit;
  static constexpr filter_type side_effect_heads       = sequenced_before_side_effect_bit;
  static constexpr filter_type tails                   = sequenced_after_mask;
  static constexpr filter_type heads                   = sequenced_before_mask;

  // Called on the tail-node of a new edge.
  void sequenced_before(Node const& head_node) const;

  // Called on the head-node of a new edge.
  void sequenced_after(Node const& tail_node) const;

  // Returns false when this Node is of the requested type (ie, any, value-computation or side-effect)
  // and is for example a head or tail (as requested) for that type, etc.
  bool is_filtered(filter_type requested_type) const;

 private:
  // A Node is stored in a std::set, but only m_id is used as sorting key.
  // Therefore all member variables that are changed after a Node was already
  // added to the set are marked mutable.
  id_type m_id;                                 // Unique identifier of a node.
  ThreadPtr m_thread;                           // The thread that this node belongs to.
  ast::tag m_variable;                          // The variable involved.
  Access m_access;                              // The type of access.
  bool m_atomic;                                // Set if this is an atomic access.
  mutable filter_type m_connected;              // Signifies existing sequenced-before relationships.
  std::memory_order m_memory_order;             // Memory order, only valid if m_atomic is true;
  std::unique_ptr<Evaluation> m_evaluation;     // The value written to m_variable -- only valid when m_access == WriteAccess.
  mutable end_points_type m_end_points;         // End points of all connected edges.

 public:
  // Non-Atomic Read.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       ast::tag const& variable) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(ReadAccess),
    m_atomic(false),
    m_connected(0),
    m_memory_order(std::memory_order_seq_cst) { }

  // Non-Atomic Write.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       ast::tag const& variable,
       Evaluation&& evaluation) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(WriteAccess),
    m_atomic(false),
    m_connected(0),
    m_memory_order(std::memory_order_seq_cst),
    m_evaluation(Evaluation::make_unique(std::move(evaluation))) { }

  // Atomic Read.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       ast::tag const& variable,
       std::memory_order memory_order) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(ReadAccess),
    m_atomic(true),
    m_connected(0),
    m_memory_order(memory_order) { }

  // Atomic Write.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       ast::tag const& variable,
       std::memory_order memory_order,
       Evaluation&& evaluation) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(WriteAccess),
    m_atomic(true),
    m_connected(0),
    m_memory_order(memory_order),
    m_evaluation(Evaluation::make_unique(std::move(evaluation))) { }

  // Other Node.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       ast::tag const& mutex,
       mutex_type access_type) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(mutex),
    m_access(convert(access_type)),
    m_atomic(false),
    m_connected(0),
    m_memory_order(std::memory_order_seq_cst) { }

  // Add a new edge of type edge_type from tail_node to head_node.
  // Returns true if such an edge did not already exist and a new edge was inserted.
  static bool add_edge(EdgeType edge_type, EndPoint::node_iterator const& tail_node, EndPoint::node_iterator const& head_node);
  void sequenced_before_side_effect_sequenced_before_value_computation() const;
  void sequenced_before_value_computation() const;

  // Accessors.
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); }
  std::string type() const;
  ast::tag tag() const { return m_variable; }
  std::string label(bool dot_file = false) const;
  ThreadPtr const thread() const { return m_thread; }
  bool is_write() const { return m_access == WriteAccess; }
  filter_type provided_type() const { return m_access == WriteAccess ? side_effect_mask : value_computation_mask; }
  bool is_second_mutex_access() const { return m_access == MutexLock2 || m_access == MutexUnlock2; }
  Evaluation* get_evaluation() { return m_evaluation.get(); }
  Evaluation const* get_evaluation() const { return m_evaluation.get(); }
  end_points_type const& get_end_points() const { return m_end_points; }

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(Node const& node1, Node const& node2) { return node1.m_id < node2.m_id; }
  friend bool operator==(Node const& node1, Node const& node2) { return node1.m_id == node2.m_id; }

  friend std::ostream& operator<<(std::ostream& os, Node const& node) { return os << node.label(); }
  struct Filter { filter_type m_connected; Filter(filter_type sb_mask) : m_connected(sb_mask) { } };
  friend std::ostream& operator<<(std::ostream& os, Filter filter);

 private:
  bool add_end_point(Edge* edge, EndPointType type, EndPoint::node_iterator const& other_node, bool edge_owner) const;
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_edge;
NAMESPACE_DEBUG_CHANNELS_END
#endif
