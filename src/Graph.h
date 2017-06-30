#pragma once

#include "utils/AIRefCount.h"
#include "utils/ulong_to_base.h"
#include "ast.h"
#include "debug.h"
#include <set>
#include <memory>
#include <iosfwd>
#include <string>

class Thread;
struct Context;

using ThreadPtr = boost::intrusive_ptr<Thread>;

class Thread : public AIRefCount
{
 public:
  using id_type = int;

 private:
  id_type m_id;         // Unique identifier of a thread.

 protected:
  Thread(id_type id) : m_id(id) { }

 public:
  static ThreadPtr create_new_thread(id_type& next_id) { return new Thread(next_id++); }
  bool is_main_thread() const { return m_id == 0; }

  friend bool operator==(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id == thr2->m_id; }
  friend bool operator!=(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id != thr2->m_id; }
  friend bool operator<(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id < thr2->m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread);
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

class Value
{
  friend std::ostream& operator<<(std::ostream& os, Value const& value);
};

class Node
{
 public:
  using id_type = int;

 private:
  id_type m_id;                         // Unique identifier of a node.
  ThreadPtr m_thread;                   // The thread that this node belongs to.
  ast::tag m_variable;                  // The variable involved.
  Access m_access;                      // The type of access.
  Value m_value;                        // The value read or written.
  bool m_atomic;                        // Set if this is an atomic access.
  std::memory_order m_memory_order;     // Memory order, only valid if m_atomic is true;
 
 public:
  Node(id_type& next_node_id,
       ThreadPtr const& thread,
       ast::tag const& mutex,
       mutex_type access_type) :
    m_id(next_node_id++),
    m_thread(thread),
    m_variable(mutex),
    m_access(convert(access_type)),
    m_atomic(false),
    m_memory_order(std::memory_order_seq_cst) { }

  Node(id_type& next_node_id, 
       ThreadPtr const& thread, 
       ast::tag const& variable) :
    m_id(next_node_id++),
    m_thread(thread),
    m_variable(variable),
    m_access(ReadAccess),
    m_atomic(false),
    m_memory_order(std::memory_order_seq_cst) { }

  Node(id_type& next_node_id, 
       ThreadPtr const& thread, 
       ast::tag const& variable, 
       Value const& value) :
    m_id(next_node_id++),
    m_thread(thread),
    m_variable(variable),
    m_access(WriteAccess),
    m_value(value),
    m_atomic(false),
    m_memory_order(std::memory_order_seq_cst) { }

  Node(id_type& next_node_id, 
       ThreadPtr const& thread, 
       ast::tag const& variable, 
       std::memory_order memory_order) :
    m_id(next_node_id++),
    m_thread(thread),
    m_variable(variable),
    m_access(ReadAccess),
    m_atomic(true),
    m_memory_order(memory_order) { }

  Node(id_type& next_node_id, 
       ThreadPtr const& thread, 
       ast::tag const& variable, 
       Value const& value,
       std::memory_order memory_order) :
    m_id(next_node_id++),
    m_thread(thread),
    m_variable(variable),
    m_access(WriteAccess),
    m_value(value),
    m_atomic(true),
    m_memory_order(memory_order) { }

  // Accessors.
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); }
  std::string type() const;

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(Node const& node1, Node const& node2) { return node1.m_id < node2.m_id; }

  friend std::ostream& operator<<(std::ostream& os, Node const& node);
};

class Graph
{
 private:
  using nodes_type = std::set<Node>;    // Use a set because we'll have a lot of iterators pointing to Node's.
  nodes_type m_nodes;                   // All nodes, ordered by Node::m_id.
  Thread::id_type m_next_thread_id;     // The id to use for the next thread; 0 is the main thread.
  ThreadPtr m_current_thread;           // The current thread.
  Node::id_type m_next_node_id;         // The id to use for the next node.

 public:
  Graph() :
    m_next_thread_id(0),
    m_current_thread{Thread::create_new_thread(m_next_thread_id)},
    m_next_node_id(0) { }

  void print_nodes() const;

 public:
  // Uninitialized declaration.
  void uninitialized(ast::tag decl, Context& context);

  // Non-atomic read and writes.
  void read(ast::tag variable, Context& context);
  void write(ast::tag variable, Context& context);

  // Atomic read and writes.
  void read(ast::tag variable, std::memory_order mo, Context& context);
  void write(ast::tag variable, std::memory_order mo, Context& context);

  // Mutex declaration and (un)locking.
  void lockdecl(ast::tag mutex, Context& context);
  void lock(ast::tag mutex, Context& context);
  void unlock(ast::tag mutex, Context& context);
};
