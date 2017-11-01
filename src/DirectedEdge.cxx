#include "sys.h"
#include "DirectedEdge.h"
#include "Action.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, DirectedEdge const& directed_edge)
{
  os << "{to:" << Action::name(directed_edge.m_linked) << "; condition:" << directed_edge.m_condition << '}';
  return os;
}
