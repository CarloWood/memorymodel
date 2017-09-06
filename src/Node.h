#pragma once

#include "Action.h"
#include "Evaluation.h"
#include "Condition.h"
#include "SBNodePresence.h"
#include <string>

// Base class for all nodes in the graph.
class NodeBase : public Action
{
 protected:
  mutable SBNodePresence m_connected;           // Signifies existing sequenced-before relationships.
  mutable boolean::Expression m_exists;         // Whether or not this node exists. Set to true until an incoming edge is added and then updated.
  static boolean::Expression const s_one;

 public:
  NodeBase() = default;
  NodeBase(id_type next_node_id, ThreadPtr const& thread, ast::tag variable) : Action(next_node_id, thread, variable), m_exists(true) { }

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
  Kind kind() const override { return non_atomic_read; }
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
  Kind kind() const override { return atomic_load; }
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
  Kind kind() const override { return non_atomic_write; }
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
  Kind kind() const override { return atomic_store; }
};

class MutexDeclNode : public NodeBase
{
 public:
  using NodeBase::NodeBase;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::side_effect; }
  Kind kind() const override { return non_atomic_write; }
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
  Kind kind() const override { return lock; }
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
  Kind kind() const override { return unlock; }
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
  Kind kind() const override { return atomic_rmw; }
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
  Kind kind() const override { return atomic_rmw; }
};

//inline
bool Edge::is_conditional() const { return !m_tail_node->exists().is_one() || !m_condition.boolean_product().is_one(); }
boolean::Expression Edge::exists() const { return m_tail_node->exists() * m_condition.boolean_product(); }
