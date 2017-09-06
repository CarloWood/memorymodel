#include "sys.h"
#include "debug.h"
#include "EdgeType.h"
#include "utils/is_power_of_two.h"
#include <iostream>

static char const* edge_str[edge_bits] =
  {
    "Sequenced-Before",
    "Additionally-Synchronizes-With",
    "Data-Dependency",
    "Control-Dependency",
    "Reads-From",
    "Total",
    "Modification-Order",
    "Sequential-Consistent",
    "Lock-Order",
    "Happens-Before",
    "Visual-Side-Effect",
    "Visible-Sequences-Of-Side-Effects",
    "Inter-Thread-Happens-Before",
    "Dependency-Ordered-Before",
    "Carries-A-Dependency-To",
    "Synchronized-With",
    "Hypothetical-Release-Sequence",
    "Release-Sequence",
    "Data-Race",
    "Unsequenced-Race"
  };

std::ostream& operator<<(std::ostream& os, EdgeType edge_type)
{
  bool first = true;
  for (int shift = 0; shift < edge_bits; ++shift)
  {
    EdgeTypePod bit = { uint32_t{1} << shift };
    if ((edge_type & bit))
    {
      if (!first)
        os << '/';
      os << edge_str[shift];
      first = false;
    }
  }
  return os;
}

static char const* edge_color[edge_bits] =
  {
    "black",
    "purple",
   "dd",
   "cd",
    "red",
   "tot",
    "blue",
   "sc",
   "lo",
   "hb",
   "vse",
   "vsses",
   "ithb",
   "dob",
   "cad",
   "sw",
   "hrs",
   "rs",
   "dr",
   "ur"
  };

char const* EdgeType::color() const
{
  ASSERT(utils::is_power_of_two(mask));
  uint32_t m = mask;
  int shift;
  for (shift = 0; m > 1; ++shift)
    m >>= 1;
  return edge_color[shift];
}

static char const* edge_name[edge_bits] =
  {
    "sb",
    "asw",
    "dd",
    "cd",
    "rf",
    "tot",
    "mo",
    "sc",
    "lo",
    "hb",
    "vse",
    "vsses",
    "ithb",
    "dob",
    "cad",
    "sw",
    "hrs",
    "rs",
    "dr",
    "ur"
  };

char const* EdgeType::name() const
{
  ASSERT(utils::is_power_of_two(mask));
  uint32_t m = mask;
  int shift;
  for (shift = 0; m > 1; ++shift)
    m >>= 1;
  return edge_name[shift];
}
