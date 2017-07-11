#pragma once

#include "ast.h"
#include "Evaluation.h"
#include "utils/AIRefCount.h"
#include "utils/ulong_to_base.h"
#include <memory>
#include <utility>
#include <iosfwd>
#include <string>

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

class Node
{
 public:
  using id_type = int;

 private:
  id_type m_id;                         // Unique identifier of a node.
  ThreadPtr m_thread;                   // The thread that this node belongs to.
  ast::tag m_variable;                  // The variable involved.
  Access m_access;                      // The type of access.
  bool m_atomic;                        // Set if this is an atomic access.
  std::memory_order m_memory_order;     // Memory order, only valid if m_atomic is true;
  std::unique_ptr<Evaluation> m_evaluation;     // The value written to m_variable -- only valid when m_access == WriteAccess.

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
    m_memory_order(std::memory_order_seq_cst) { }

  // Accessors.
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); }
  std::string type() const;
  ast::tag tag() const { return m_variable; }
  bool is_write() const { return m_access == WriteAccess; }
  Evaluation const* get_evaluation() const { return m_evaluation.get(); }

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(Node const& node1, Node const& node2) { return node1.m_id < node2.m_id; }

  friend std::ostream& operator<<(std::ostream& os, Node const& node);
};
