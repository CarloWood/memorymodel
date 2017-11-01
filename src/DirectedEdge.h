#pragma once

#include "boolean-expression/BooleanExpression.h"
#include <iosfwd>

class DirectedEdge
{
 private:
  int m_linked;                         // The node that this edge is linked to.
  boolean::Expression m_condition;      // The condition under which this edge exists.

 public:
  DirectedEdge(int linked, boolean::Expression const& condition) : m_linked(linked), m_condition(condition.copy()) { }

  friend std::ostream& operator<<(std::ostream& os, DirectedEdge const& directed_edge);
};
