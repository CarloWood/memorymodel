#pragma once

#include "Property.h"
#include "Propagator.h"
#include "ast_tag.h"
#include "boolean-expression/BooleanExpression.h"
#include <vector>
#include <iosfwd>

class ReadFromGraph;

class Properties
{
 private:
  using map_type = std::vector<Property>;
  map_type m_map;       // Called map because each Property only occurs once.

 public:
  void reset() { m_map.clear(); }
  bool empty() const { return m_map.empty(); }
  void add(Property&& property);
  void merge(Properties const& properties, Propagator const& propagator, ReadFromGraph const* read_from_graph);
  bool contains_relevant_property(ReadFromGraph const* read_from_graph) const;
  boolean::Expression current_loop_condition(ReadFromGraph const* read_from_graph);
  void copy_to(Property& rs_property) const;

  friend std::ostream& operator<<(std::ostream& os, Properties const& properties);
};
