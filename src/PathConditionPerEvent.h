#pragma once

#include "Event.h"
#include "ast_tag.h"
#include "boolean-expression/BooleanExpression.h"
#include <vector>
#include <iosfwd>

class ReadFromGraph;

class PathConditionPerEvent
{
 private:
  using map_type = std::vector<std::pair<Event, boolean::Expression>>;
  map_type m_map;       // Called map because each Event only occurs once.

 public:
  void reset() { m_map.clear(); }
  void add_new(Event const& event, boolean::Expression&& condition);
  void add_new(PathConditionPerEvent const& path_condition_per_event, boolean::Expression const& condition, ReadFromGraph const* read_from_graph);
  bool contains_relevant_event(ReadFromGraph const* read_from_graph) const;
  bool empty() const { return m_map.empty(); }
  boolean::Expression current_loop_condition(SequenceNumber end_point);
  void set_hidden(ast::tag location, TopologicalOrderedActions const& topological_ordered_actions);

  friend std::ostream& operator<<(std::ostream& os, PathConditionPerEvent const& path_condition_per_event);
};
