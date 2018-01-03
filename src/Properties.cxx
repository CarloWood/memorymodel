#include "sys.h"
#include "Properties.h"
#include "ReadFromGraph.h"

void Properties::add(Property&& new_property)
{
  if (!new_property.path_condition().is_zero()) // Speed up.
    new_property.merge_into(m_map);
}

// Merge properties into this object.
void Properties::merge(
    Properties const& properties,
    Propagator const& propagator,
    ReadFromGraph const* read_from_graph)
{
  // Add the new properties to our map, updating their path condition,
  // and convert them according to the propagator.
  for (Property const& property : properties.m_map)
    if (property.is_relevant(*read_from_graph))
    {
      // Create a new Property in m_map from property but with already updated path condition.
      Property new_property(property, property.path_condition().times(propagator.condition()));
      // Then apply the propagator to the rest of the data.
      if (new_property.convert(propagator))
        add(std::move(new_property));
    }
}

bool Properties::contains_relevant_property(ReadFromGraph const* read_from_graph) const
{
  for (Property const& property : m_map)
    if (property.is_relevant(*read_from_graph))
      return true;
  return false;
}

// Return the condition under which the properties, that have the current node as end point, invalidate the graph.
boolean::Expression Properties::current_loop_condition(ReadFromGraph const* read_from_graph)
{
  boolean::Expression invalid_condition{false};
  for (Property const& property : m_map)
    if (property.invalidates_graph(read_from_graph))
    {
      Dout(dc::readfrom, "Property " << property << " invalidates graph under condition " << property.path_condition() << '.');
      invalid_condition += property.path_condition();
    }
  return invalid_condition;
}

std::ostream& operator<<(std::ostream& os, Properties const& properties)
{
  os << '{';
  char const* separator = "";
  for (auto&& property : properties.m_map)
  {
    os << separator << property;
    separator = ", ";
  }
  return os << '}';
}
