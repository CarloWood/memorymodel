#pragma once

#include "Action.h"
#include "Evaluation.h"
#include "Condition.h"
#include <string>

// Base class for value-computation nodes.
class ReadNode : public Action
{
 public:
  using Action::Action;

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
class WriteNode : public Action
{
 protected:
  std::unique_ptr<Evaluation> m_evaluation;     // The value written to m_location.

 public:
  WriteNode(id_type next_node_id, ThreadPtr const& thread, ast::tag const& variable, Evaluation&& evaluation) :
    Action(next_node_id, thread, variable), m_evaluation(Evaluation::make_unique(std::move(evaluation))) { }

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

class MutexDeclNode : public Action
{
 public:
  using Action::Action;

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

class MutexLockNode : public Action
{
 public:
  using Action::Action;

  // Interface implementation.
  std::string type() const override;
  void print_code(std::ostream& os) const override;
  NodeProvidedType provided_type() const override { return NodeProvidedType::value_computation_and_side_effect; }
  bool is_second_mutex_access() const override { return true; }
  Kind kind() const override { return lock; }
};

class MutexUnlockNode : public Action
{
 public:
  using Action::Action;

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
bool Edge::is_conditional() const { return !m_condition.is_one(); }
boolean::Expression Edge::exists() const { return m_condition * m_tail_node->exists().as_product(); }
