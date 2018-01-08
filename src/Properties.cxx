#include "sys.h"
#include "Properties.h"
#include "ReadFromGraph.h"

void Properties::add(Property&& new_property)
{
  DoutEntering(dc::property, "Properties::add(" << new_property << ").");
  if (!new_property.path_condition().is_zero()) // Speed up.
    new_property.merge_into(m_map);
}

// Merge properties into this object.
void Properties::merge(
    Properties const& properties,
    Propagator const& propagator,
    ReadFromGraph const* read_from_graph)
{
  DoutEntering(dc::property, "Properties::merge(" << properties << ", " << propagator << ", read_from_graph)");
  if (propagator.rf_acq_but_not_rel())
  {
    Dout(dc::property, "Unsynced Release-Sequence detected.");
    // This signifies the start of a Release-Sequence. We need to replace the map of properties with
    // one that contains a single release_sequence Property that wraps everything it contained before.
    Property new_property(release_sequence, propagator.child(), propagator.condition());
    properties.copy_to(new_property, propagator.current_node());
    // Then apply the propagator to the rest of the data.
    if (new_property.convert(propagator))
      add(std::move(new_property));
  }
  // Add the new properties to our map, updating their path condition,
  // and convert them according to the propagator.
  for (Property const& property : properties.m_map)
    if (property.is_relevant(*read_from_graph))
    {
      // Create a new Property in m_map from property but with already updated path condition.
      Property new_property(property, property.path_condition().times(propagator.condition()));
      // Then apply the propagator to the rest of the data.
      if (new_property.convert(propagator) && !new_property.if_needed_unwrap_to(*this))
        add(std::move(new_property));
    }
}

void Properties::copy_to(Property& rs_property, SequenceNumber current_node) const
{
  DoutEntering(dc::property, "Properties::copy_to(" << rs_property << ").");
  for (Property const& property : m_map)
    if (!(property.type() == reads_from && property.end_point() == current_node))
      rs_property.wrap(property);
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
