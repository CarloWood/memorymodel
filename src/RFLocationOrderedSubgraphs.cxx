#include "sys.h"
#include "RFLocationOrderedSubgraphs.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, RFLocation index)
{
  os << 'L' << index.get_value();
  return os;
}
