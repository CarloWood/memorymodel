#include "sys.h"
#include "RFLocationOrderedSubgraphs.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, RFLocation index)
{
  if (index.undefined())
    os << "L?";
  else
    os << 'L' << index.get_value();
  return os;
}
