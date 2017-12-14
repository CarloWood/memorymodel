#include "sys.h"
#include "TopologicalOrderedActions.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, TopologicalOrderedActionsIndex index)
{
  return os << '#' << (index.get_value() + 1);
}
