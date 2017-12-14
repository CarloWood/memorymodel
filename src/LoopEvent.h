#pragma once

#include <iosfwd>
#include "TopologicalOrderedActions.h"

class ReadFromGraph;

enum loop_event_type {
  causal_loop,
  hidden_vse
};

class LoopEvent
{
 private:
  loop_event_type m_loop_event;                 // The type of event.
  SequenceNumber m_end_point;   // The first vertex that was found that was already visited while following this path.

 public:
  LoopEvent(loop_event_type loop_event, SequenceNumber end_point) : m_loop_event(loop_event), m_end_point(end_point) { }

  // Return true when this event is actual at this moment in the Depth-First-Search of read_from_graph.
  bool is_actual(ReadFromGraph const& read_from_graph) const;

  // Accessor.
  SequenceNumber end_point() const { return m_end_point; }

  friend bool operator==(LoopEvent const& le1, LoopEvent const& le2)
  {
    return le1.m_end_point == le2.m_end_point && le1.m_loop_event == le2.m_loop_event;
  }

  friend std::ostream& operator<<(std::ostream& os, LoopEvent const& loop_event);
};
