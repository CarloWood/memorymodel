#pragma once

#include "Thread.h"
#include "ast_tag.h"
#include "Evaluation.h"
#include "Condition.h"
#include "SBNodePresence.h"
#include "NodePtr.h"
#include "Location.h"
#include "utils/ulong_to_base.h"
#include <boost/intrusive_ptr.hpp>
#include <string>

class Edge;

enum mutex_type
{
  mutex_decl,
  mutex_lock1,
  mutex_lock2,
  mutex_unlock1,
  mutex_unlock2
};

enum EdgeType {
  edge_sb,               // Sequenced-Before.
  edge_asw,              // Additional-Synchronizes-With.
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
  edge_sw,               // Synchronizes-with.
  edge_hrs,              // Hypothetical release sequence.
  edge_rs,               // Release sequence.
  edge_dr,               // Inter-thread data races, unrelated by hb.
  edge_ur                // Intra-thread unsequenced races, unrelated by sb.
};

char const* edge_str(EdgeType edge_type);
std::ostream& operator<<(std::ostream& os, EdgeType edge_type);

inline bool is_directed(EdgeType edge_type)
{
  // FIXME
  return edge_type < edge_dr;
}

enum EndPointType {
  undirected,
  tail,                 // >>-- This end point is the tail of the arrow: the beginning of the directed edge.
  head                  // -->> This end point is the head of the arrow: the end of the directed edge.
};

char const* end_point_str(EndPointType end_point_type);
std::ostream& operator<<(std::ostream& os, EndPointType end_point_type);

// One side of a given edge.
class EndPoint
{
 private:
  Edge* m_edge;
  EndPointType m_type;
  NodePtr m_other_node;
  bool m_edge_owner;            // Owns the allocation of m_edge.

 public:
  EndPoint(Edge* edge, EndPointType type, NodePtr const& other_node, bool edge_owner) :
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
  inline ~EndPoint();

  // Accessors.
  inline EdgeType edge_type() const;
  Edge* edge() const { return m_edge; }
  EndPointType type() const { return m_type; }
  NodePtr other_node() const { return m_other_node; }

  friend bool operator==(EndPoint const& end_point1, EndPoint const& end_point2);
  friend std::ostream& operator<<(std::ostream& os, EndPoint const& end_point);
};

class Edge
{
 private:
  EdgeType m_edge_type;
  Condition m_condition;
  NodePtr m_tail_node;    // The Node from where the edge starts:  tail_node ---> head_node.
#ifdef CWDEBUG
  int m_id;             // For debugging purposes.
  static int s_id;
 public:
  int id() const { return m_id; }
#endif

 public:
  Edge(EdgeType edge_type, NodePtr const& tail_node, Condition const& condition) :
      m_edge_type(edge_type),
      m_condition(condition),
      m_tail_node(tail_node)
      COMMA_DEBUG_ONLY(m_id(s_id++))
      {
        Dout(dc::sb_edge(edge_type == edge_sb)|
             dc::asw_edge(edge_type == edge_asw),
             "Creating " << edge_type << " Edge " << m_id << '.');
      }

  EdgeType edge_type() const { return m_edge_type; }
  Condition const& condition() const { return m_condition; }
  char const* name() const;

  inline bool is_conditional() const;
  inline boolean::Expression exists() const;

  friend std::ostream& operator<<(std::ostream& os, Edge const& edge);
  friend bool operator==(Edge const& edge1, Edge const& edge2) { return edge1.m_edge_type == edge2.m_edge_type; }
};

//inline -- now that Edge is defined we can define this.
EdgeType EndPoint::edge_type() const { return m_edge->edge_type(); }
EndPoint::~EndPoint() { if (m_edge_owner) delete m_edge; }

// Base class for all nodes in the graph.
class NodeBase
{
 public:
  using id_type = int;
  using end_points_type = std::vector<EndPoint>;

 protected:
  // A node is stored in a std::set, but only m_id is used as sorting key.
  // Therefore all member variables that are changed after a node was already
  // added to the set are marked mutable.
  id_type m_id;                                 // Unique identifier of a node.
  ThreadPtr m_thread;                           // The thread that this node belongs to.
  locations_type::const_iterator m_location;    // The variable/mutex involved.
  mutable SBNodePresence m_connected;           // Signifies existing sequenced-before relationships.
  mutable end_points_type m_end_points;         // End points of all connected edges.
  mutable boolean::Expression m_exists; // Whether or not this node exists. Set to true until an incoming edge is added and then updated.
  static boolean::Expression const s_one;

 public:
  NodeBase() = default;
  NodeBase(id_type next_node_id, ThreadPtr const& thread, ast::tag variable);
  NodeBase(NodeBase const&) = delete;
  NodeBase(NodeBase&&) = delete;
  virtual ~NodeBase() = default;

  virtual bool is_second_mutex_access() const { return false; }
  virtual std::string type() const = 0;
  virtual void print_code(std::ostream& os) const = 0;
  virtual NodeProvidedType provided_type() const = 0;

 public:
  // Called on the tail-node of a new (conditional) sb edge.
  void sequenced_before() const;

  // Called on the head-node of a new (conditional) sb edge.
  void sequenced_after() const;

  // Returns true when this node is not of the requested type (ie, any, value-computation or side-effect)
  // or is not a head or tail (if requested) for that type, and is neither hiding behind another
  // node of such type. However, hiding might be set to a boolean expression that is not TRUE (1),
  // if this node is hiding conditionally behind the requested type.
  bool matches(NodeRequestedType const& requested_type, boolean::Expression& hiding) const;

  boolean::Expression const& provides_sequenced_before_value_computation() const;
  boolean::Expression const& provides_sequenced_before_side_effect() const;
  bool provides_sequenced_after_something() const;

  // Add a new edge of type edge_type from tail_node to head_node.
  // Returns true if such an edge did not already exist and a new edge was inserted.
  static bool add_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Condition const& condition);
  void sequenced_before_side_effect_sequenced_before_value_computation() const;
  void sequenced_before_value_computation() const;

  // Accessors.
  ast::tag tag() const { return m_location->tag(); }
  Location const& location() const { return *m_location; }
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); } // action_id
  ThreadPtr const thread() const { return m_thread; }
  end_points_type const& get_end_points() const { return m_end_points; }
  boolean::Expression const& exists() const { return m_exists; }

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(NodeBase const& node1, NodeBase const& node2) { return node1.m_id < node2.m_id; }
  friend bool operator==(NodeBase const& node1, NodeBase const& node2) { return node1.m_id == node2.m_id; }

  friend std::ostream& operator<<(std::ostream& os, NodeBase const& node);

 private:
  bool add_end_point(Edge* edge, EndPointType type, NodePtr const& other_node, bool edge_owner) const;
  void update_exists() const;
};

// Base class for value-computation nodes.
class ReadNode : public NodeBase
{
 public:
  using NodeBase::NodeBase;

  // Interface implementation.
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation; }
};

class NAReadNode : public ReadNode
{
 public:
  using ReadNode::ReadNode;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
};

class AtomicReadNode : public ReadNode
{
 private:
  std::memory_order m_read_memory_order;             // Memory order.

 public:
  AtomicReadNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable, std::memory_order memory_order) :
      ReadNode(next_node_id, thread, variable), m_read_memory_order(memory_order) { }

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
};

// Base class for [value-computation/]side-effect nodes that write m_evaluation to their memory location.
class WriteNode : public NodeBase
{
 protected:
  std::unique_ptr<Evaluation> m_evaluation;     // The value written to m_location.

 public:
  WriteNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable, Evaluation&& evaluation) :
    NodeBase(next_node_id, thread, variable), m_evaluation(Evaluation::make_unique(std::move(evaluation))) { }

  // Accessors.
  Evaluation* get_evaluation() { return m_evaluation.get(); }
  Evaluation const* get_evaluation() const { return m_evaluation.get(); }

  // Interface implementation.
  NodeProvidedType provided_type() const override { return NodeProvidedType::side_effect; }
  void print_code(std::ostream& os) const override;
};

class NAWriteNode : public WriteNode
{
 public:
  using WriteNode::WriteNode;

  // Interface implementation.
  std::string type() const override;
};

class AtomicWriteNode : public WriteNode
{
 protected:
  std::memory_order m_write_memory_order;             // Memory order.

 public:
  AtomicWriteNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable, std::memory_order memory_order, Evaluation&& evaluation) :
      WriteNode(next_node_id, thread, variable, std::move(evaluation)), m_write_memory_order(memory_order) { }

  // Interface implementation.
  std::string type() const override;
};

class MutexDeclNode : public NodeBase
{
 public:
  using NodeBase::NodeBase;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::side_effect; }
};

class MutexReadNode : public NAReadNode
{
 public:
  using NAReadNode::NAReadNode;

  // Interface implementation.
  void print_code(std::ostream& os) const override;
};

class MutexLockNode : public NodeBase
{
 public:
  using NodeBase::NodeBase;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation_and_side_effect; }
  bool is_second_mutex_access() const override { return true; }
};

class MutexUnlockNode : public NodeBase
{
 public:
  using NodeBase::NodeBase;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation_and_side_effect; }
  bool is_second_mutex_access() const override { return true; }
};

class RMWNode : public WriteNode
{
 private:
  std::memory_order m_memory_order;

 public:
  RMWNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable, std::memory_order memory_order, Evaluation&& evaluation) :
      WriteNode(next_node_id, thread, variable, std::move(evaluation)), m_memory_order(memory_order) { }

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation_and_side_effect; }
};

class CEWNode : public AtomicWriteNode
{
 private:
   std::memory_order m_fail_memory_order;
   ast::tag m_expected;
   int m_desired;

 public:
  CEWNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable,
          ast::tag const& expected, int desired, std::memory_order success_memory_order, std::memory_order fail_memory_order, Evaluation&& evaluation) :
      AtomicWriteNode(next_node_id, thread, variable, success_memory_order, std::move(evaluation)),
      m_fail_memory_order(fail_memory_order), m_expected(expected), m_desired(desired) { }

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation_and_side_effect; }
};

//inline
bool Edge::is_conditional() const { return !m_tail_node->exists().is_one() || !m_condition.boolean_product().is_one(); }
boolean::Expression Edge::exists() const { return m_tail_node->exists() * m_condition.boolean_product(); }
