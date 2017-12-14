#include "sys.h"
#include "TopologicalOrderedActions.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, SequenceNumber index)
{
  return os << '#' << (index.get_value() + 1);
}
