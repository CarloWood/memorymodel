#pragma once

#include <iosfwd>
#include "TopologicalOrderedActions.h"
#include "RFLocationOrderedSubgraphs.h"
#include "Action.h"
#include "boolean-expression/BooleanExpression.h"

class Propagator
{
 private:
  Action* m_current_action;
  Action* m_child_action;
  SequenceNumber m_current_node;
  SequenceNumber m_child;
  RFLocation m_current_location;
  boolean::Expression const& m_condition;
  bool m_current_is_write;                      // Also false when the memory location of the current node is not to be considered as node at the moment.
  bool m_edge_is_rf;

 public:
  Propagator(
      Action* current_action,
      SequenceNumber current_node,
      RFLocation current_location,
      bool current_is_write,
      Action* child_action,
      SequenceNumber child,
      bool edge_is_rf,
      boolean::Expression const& condition) :
      m_current_action(current_action),
      m_child_action(child_action),
      m_current_node(current_node),
      m_child(child),
      m_current_location(current_location),
      m_condition(condition),
      m_current_is_write(current_is_write),
      m_edge_is_rf(edge_is_rf) { }

  // Return the condition under which this propagation will take place.
  boolean::Expression const& condition() const { return m_condition; }
  bool edge_is_rf() const { return m_edge_is_rf; }
  RFLocation current_location() const { return m_current_location; }
  Thread::id_type current_thread() const { return m_current_action->thread()->id(); }

  bool rf_acq_but_not_rel() const;
  bool is_write_rel_to(RFLocation location) const;
  bool is_non_rel_write(RFLocation location) const;

  friend std::ostream& operator<<(std::ostream& os, Propagator const&);
};
