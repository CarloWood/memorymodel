#pragma once

#include "Edge.h"
#include "Location.h"
#include "ast_tag.h"
#include "Thread.h"
#include "SBNodePresence.h"
#include "ActionSet.h"
#include "utils/ulong_to_base.h"
#include <vector>
#include <memory>

class Graph;

// Base class for all nodes in the graph.
class Action
{
 public:
  using id_type = int;
  using end_points_type = std::vector<EndPoint>;

  enum Kind
  {
    lock,
    unlock,
    atomic_load,
    atomic_store,
    atomic_rmw,
    non_atomic_read,
    non_atomic_write,
    fence
  };

  static char const* action_kind_str(Kind kind);
  friend std::ostream& operator<<(std::ostream& os, Kind const& kind) { return os << action_kind_str(kind); }

 protected:
  // A node is stored in a std::set, but only m_id is used as sorting key.
  // Therefore all member variables that are changed after a node was already
  // added to the set are marked mutable.
  id_type m_id;                                 // Unique identifier.
  ThreadPtr m_thread;                           // The thread that this action belongs to.
  locations_type::const_iterator m_location;    // The variable/mutex involved.
  // Graph information.
  end_points_type m_end_points;                 // End points of all connected edges.
  boolean::Expression m_exists;                 // Whether or not this node exists. Set to true until an incoming edge is added and then updated.
  SBNodePresence m_connected;                   // Signifies existing sequenced-before relationships.
  static boolean::Expression const s_expression_one;
  static boolean::Product const s_product_one;

  // Post opsem stuff.
  int m_sequence_number;                        // Some over all ordering number (n) such that if A--sb/asw-->B than n_A < n_B.
  ActionSet m_prior_actions;                    // Actions from which this action is reachable through SB and ASW edges.
  int m_read_from_loop_index;                   // The index into the ... array

 public:
  Action() = default;
  Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable);
  Action(Action const&) = delete;
  Action(Action&&) = delete;
  virtual ~Action() = default;

  // Add a new edge of type edge_type from this Action node to head, that exists if condition is true.
  void add_edge_to(EdgeType edge_type, Action* head_node, boolean::Expression&& condition = boolean::Expression{true});

  // Delete edge added before by add_edge_to.
  void delete_edge_to(EdgeType edge_type, Action* head_node);

  // Called on the tail-node of a new (conditional) sb edge.
  void sequenced_before();

  // Called on the head-node of a new (conditional) sb edge.
  void sequenced_after();

  // Returns true when this node is not of the requested type (ie, any, value-computation or side-effect)
  // or is not a head or tail (if requested) for that type, and is neither hiding behind another
  // node of such type. However, hiding might be set to a boolean expression that is not TRUE (1),
  // if this node is hiding conditionally behind the requested type.
  bool matches(NodeRequestedType const& requested_type, boolean::Expression& hiding) const;

  boolean::Expression const& provides_sequenced_before_value_computation() const;
  boolean::Expression const& provides_sequenced_before_side_effect() const;
  bool provides_sequenced_after_something() const;

  void sequenced_before_side_effect_sequenced_before_value_computation();
  void sequenced_before_value_computation();

  bool is_read() const { Kind kind_ = kind(); return kind_ == atomic_load || kind_ == atomic_rmw || kind_ == non_atomic_read; }
  bool is_write() const { Kind kind_ = kind(); return kind_ == atomic_store || kind_ == atomic_rmw || kind_ == non_atomic_write; }

  Edge* first_of(EndPointType end_point_type, EdgeMaskType edge_mask_type) const
  {
    for (auto&& end_point : m_end_points)
      if (end_point.type() == end_point_type && (end_point.edge_type() & edge_mask_type))
        return end_point.edge();
    return nullptr;
  }

  template<class FOLLOW, class FILTER>
  void for_actions_no_condition(
    FOLLOW follow,
    FILTER filter,
    std::function<bool(Action*)> const& if_found) const;

  // Returns an expression that is true when something was found.
  template<class FOLLOW, class FILTER>     // bool FOLLOW::operator()(EndPoint const&) and FILTER::operator()(Action const&) must exist.
  void for_actions(
    FOLLOW follow,            // Follow each EndPoint when `bool FOLLOW::operator()(EndPoint const&)` returns true.
    FILTER filter,            // Call if_found() for each action found after following an edge when `bool FILTER::operator()(Action const&)` returns true.
                              // Call for_actions() recursively unless if_found returned true (so if if_found wasn't called, then always call for_actions).
    std::function<bool(Action*, boolean::Product const&)> const& if_found,
    boolean::Product const& path_condition = boolean::Product{true}) const;     // The product of the edge conditions encountered.

  // Accessors.
  id_type id() const { return m_id; }
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); } // action_id
  ThreadPtr const thread() const { return m_thread; }
  ast::tag tag() const { return m_location->tag(); }
  Location const& location() const { return *m_location; }
  end_points_type const& get_end_points() const { return m_end_points; }
  boolean::Expression const& exists() const { return m_exists; }

  // Post opsem stuff.
  static void initialize_post_opsem(Graph const& graph, std::vector<Action*>& topological_ordered_actions);
  int sequence_number() const { return m_sequence_number; }
  bool is_sequenced_before(Action const& action) const { return action.m_prior_actions.includes(*this); }
  void set_read_from_loop_index(int read_from_loop_index) { m_read_from_loop_index = read_from_loop_index; }
  int get_read_from_loop_index() const { return m_read_from_loop_index; }
  boolean::Expression is_fully_visited(int visited_generation, boolean::Product const& path_condition) const;

  virtual Kind kind() const = 0;
  virtual bool is_second_mutex_access() const { return false; }
  virtual std::string type() const = 0;
  virtual void print_code(std::ostream& os) const = 0;
  virtual NodeProvidedType provided_type() const = 0;

  // Less-than comparator for Graph::m_nodes.
  friend bool operator<(Action const& action1, Action const& action2) { return action1.m_id < action2.m_id; }
  friend bool operator==(Action const& action1, Action const& action2) { return action1.m_id == action2.m_id; }

  void print_node_label_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, Action const& action);

 protected:
  void add_end_point(Edge* edge, EndPointType type, Action* other_node, bool edge_owner);
  void update_exists();
};

#include "ActionSet.inl"
