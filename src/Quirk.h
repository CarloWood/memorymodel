#pragma once

#include <iosfwd>
#include "TopologicalOrderedActions.h"

class ReadFromGraph;

enum quirk_type {
  causal_loop,
  reads_from
};

class Quirk
{
 private:
  quirk_type m_quirk;           // The type of quirk.
  SequenceNumber m_end_point;   // causal_loop: the first node that was found that was already visited while following this path.
                                // reads_from : the node that is being read.
  // Only valid for reads_from:
  bool m_hidden;                // Set to true when a write to the corresponding memory location will happen before reaching m_end_point.

 public:
  Quirk(quirk_type quirk, SequenceNumber end_point) : m_quirk(quirk), m_end_point(end_point), m_hidden(false) { }

  // Called for an Quirk of type reads_from when returning from the dfs() call that detected
  // this Quirk to a node unequal the end point itself that does a write to the corresponding memory location.
  void set_hidden() { m_hidden = true; }

  // Return true when this quirk is relevant to be copied from child to parent node.
  bool is_relevant(ReadFromGraph const& read_from_graph) const;

  // Accessors.
  quirk_type type() const { return m_quirk; }
  SequenceNumber end_point() const { return m_end_point; }
  bool is_hidden() const { return m_hidden; }

  friend bool operator==(Quirk const& le1, Quirk const& le2)
  {
    return le1.m_end_point == le2.m_end_point && le1.m_quirk == le2.m_quirk && le1.m_hidden == le2.m_hidden;
  }

  friend std::ostream& operator<<(std::ostream& os, Quirk const& quirk);
};
