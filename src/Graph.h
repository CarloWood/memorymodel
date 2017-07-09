#pragma once

#include "utils/AIRefCount.h"
#include "utils/ulong_to_base.h"
#include "ast.h"
#include "debug.h"
#include <set>
#include <memory>
#include <iosfwd>
#include <string>
#include <stack>
#include <vector>

class Thread;
struct Context;

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

 public:
  // Non-Atomic Read or Write.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       bool write,
       ast::tag const& variable) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(write ? WriteAccess : ReadAccess),
    m_atomic(false),
    m_memory_order(std::memory_order_seq_cst) { }

  // Atomic Read or Write.
  Node(id_type next_node_id,
       ThreadPtr const& thread,
       bool write,
       ast::tag const& variable,
       std::memory_order memory_order) :
    m_id(next_node_id),
    m_thread(thread),
    m_variable(variable),
    m_access(write ? WriteAccess : ReadAccess),
    m_atomic(true),
    m_memory_order(memory_order) { }

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

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(Node const& node1, Node const& node2) { return node1.m_id < node2.m_id; }
};

// Hack to print Nodes.
struct NodeWithContext
{
  Node const& m_node;
  Context& m_context;

  NodeWithContext(Node const& node, Context& context) : m_node(node), m_context(context) { }

  friend std::ostream& operator<<(std::ostream& os, NodeWithContext const& node);
};

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

  void print_nodes(Context& context) const;

 public:
  // Entering and leaving scopes.
  void scope_start(bool is_thread);
  void scope_end();

  // A new node was added.
  template<typename ...Args>
  void new_node(Context& context, Args... args)
  {
    DebugMarkUp;
    auto node = m_nodes.emplace_hint(m_nodes.end(), m_next_node_id++, m_current_thread, args...);
    Dout(dc::notice, "Created node " << NodeWithContext(*node, context) << '.');
  }
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_barrier;
extern channel_ct edge;
NAMESPACE_DEBUG_CHANNELS_END
#endif
