#pragma once

#include "Edge.h"
#include "Location.h"
#include "ast_tag.h"
#include "Thread.h"
#include "utils/ulong_to_base.h"
#include <vector>
#include <memory>

// Base class of NodeBase.
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
  mutable end_points_type m_end_points;         // End points of all connected edges.
  mutable boolean::Expression m_exists;         // Whether or not this node exists. Set to true until an incoming edge is added and then updated.
 
 public:
  Action() = default;
  Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable);
  Action(Action const&) = delete;
  Action(Action&&) = delete;
  virtual ~Action() = default;

  void add_edge_to(EdgeType edge_type, Action const& head) const; // Const because Actions are stored in a set.

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
    std::function<bool(Action const&)> const& if_found) const;

  // Returns an expression that is true when something was found.
  template<class FOLLOW, class FILTER>     // bool FOLLOW::operator()(EndPoint const&) and FILTER::operator()(Action const&) must exist.
  boolean::Expression for_actions(
    FOLLOW follow,            // Follow each EndPoint when `bool FOLLOW::operator()(EndPoint const&)` returns true.
    FILTER filter,            // Call if_found() for each action found after following an edge when `bool FILTER::operator()(Action const&)` returns true.
                              // Call for_actions() recursively unless if_found returned true (so if if_found wasn't called, then always call for_actions).
    std::function<bool(Action const&)> const& if_found) const;

  // Accessors.
  std::string name() const { return utils::ulong_to_base(m_id, "abcdefghijklmnopqrstuvwxyz"); } // action_id
  ThreadPtr const thread() const { return m_thread; }
  ast::tag tag() const { return m_location->tag(); }
  Location const& location() const { return *m_location; }
  end_points_type const& get_end_points() const { return m_end_points; }
  boolean::Expression const& exists() const { return m_exists; }
  virtual Kind kind() const = 0;

  friend std::ostream& operator<<(std::ostream& os, Action const& action);

 protected:
  void add_end_point(Edge* edge, EndPointType type, NodeBase const* other_node, bool edge_owner) const;  // const because Actions are stored in a set.
};
