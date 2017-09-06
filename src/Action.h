#pragma once

#include "Edge.h"
#include "Location.h"
#include "ast_tag.h"
#include "Thread.h"
#include <vector>
#include <memory>

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
 
 public:
  Action() = default;
  Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable);
  Action(Action const&) = delete;
  Action(Action&&) = delete;
  virtual ~Action() = default;

  Location const& location() const { return *m_location; }
  bool is_read() const { Kind kind_ = kind(); return kind_ == atomic_load || kind_ == atomic_rmw || kind_ == non_atomic_read; }
  bool is_write() const { Kind kind_ = kind(); return kind_ == atomic_store || kind_ == atomic_rmw || kind_ == non_atomic_write; }

  Edge* first_of(EndPointType end_point_type, EdgeType edge_type) const
  {
    for (auto&& end_point : m_end_points)
      if (end_point.type() == end_point_type && (end_point.edge_type() & edge_type))
        return end_point.edge();
    return nullptr;
  }

  template<typename FOLLOW>     // bool FOLLOW::operator()(EndPoint const&) must exist.
  bool for_actions(
    FOLLOW follow,
    std::function<bool(Action const&)> const& filter,
    std::function<bool(Action const&)> const& found) const;

  // Accessors.
  virtual Kind kind() const = 0;

  friend std::ostream& operator<<(std::ostream& os, Action const& action);
};
