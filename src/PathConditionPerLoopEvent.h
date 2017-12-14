#pragma once

#include "LoopEvent.h"
#include "boolean-expression/BooleanExpression.h"
#include <vector>
#include <iosfwd>

class ReadFromGraph;

class PathConditionPerLoopEvent
{
 private:
  std::vector<std::pair<LoopEvent, boolean::Expression>> m_map;    // Called map because each LoopEvent only occurs once.

 public:
  void reset() { m_map.clear(); }
  void add_new(LoopEvent const& loop_event, boolean::Expression&& condition);
  void add_new(PathConditionPerLoopEvent const& path_condition_per_loop_event, boolean::Expression const& condition, ReadFromGraph const* read_from_graph);
  bool contains_actual_loop_event(ReadFromGraph const* read_from_graph) const;
  bool empty() const { return m_map.empty(); }
  boolean::Expression current_loop_condition(TopologicalOrderedActionsIndex end_point) const;

  friend std::ostream& operator<<(std::ostream& os, PathConditionPerLoopEvent const& path_condition_per_loop_event);
};
