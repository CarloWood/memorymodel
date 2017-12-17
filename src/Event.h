#pragma once

#include <iosfwd>
#include "TopologicalOrderedActions.h"

class ReadFromGraph;

enum event_type {
  causal_loop,
  reads_from
};

class Event
{
 private:
  event_type m_event;           // The type of event.
  SequenceNumber m_end_point;   // causal_loop: the first node that was found that was already visited while following this path.
                                // reads_from : the node that is being read.
  // Only valid for reads_from:
  bool m_hidden;                // Set to true when a write to the corresponding memory location will happen before reaching m_end_point.

 public:
  Event(event_type event, SequenceNumber end_point) : m_event(event), m_end_point(end_point), m_hidden(false) { }

  // Called for an Event of type reads_from when returning from the dfs() call that detected
  // this Event to a node unequal the end point itself that does a write to the corresponding memory location.
  void set_hidden() { m_hidden = true; }

  // Return true when this event is relevant to be copied from child to parent node.
  bool is_relevant(ReadFromGraph const& read_from_graph) const;

  // Accessors.
  event_type type() const { return m_event; }
  SequenceNumber end_point() const { return m_end_point; }
  bool is_hidden() const { return m_hidden; }

  friend bool operator==(Event const& le1, Event const& le2)
  {
    return le1.m_end_point == le2.m_end_point && le1.m_event == le2.m_event && le1.m_hidden == le2.m_hidden;
  }

  friend std::ostream& operator<<(std::ostream& os, Event const& event);
};
