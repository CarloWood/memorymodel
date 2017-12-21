#pragma once

#include "Quirk.h"
#include "ast_tag.h"
#include "boolean-expression/BooleanExpression.h"
#include <vector>
#include <iosfwd>

class ReadFromGraph;

class Properties
{
 private:
  using map_type = std::vector<std::pair<Quirk, boolean::Expression>>;
  map_type m_map;       // Called map because each Quirk only occurs once.

 public:
  void reset() { m_map.clear(); }
  void add_new(Quirk const& quirk, boolean::Expression&& condition);
  void add_new(Properties const& properties, boolean::Expression const& condition, ReadFromGraph const* read_from_graph);
  bool contains_relevant_quirk(ReadFromGraph const* read_from_graph) const;
  bool empty() const { return m_map.empty(); }
  boolean::Expression current_loop_condition(SequenceNumber end_point);
  void set_hidden(ast::tag location, TopologicalOrderedActions const& topological_ordered_actions);

  friend std::ostream& operator<<(std::ostream& os, Properties const& properties);
};
