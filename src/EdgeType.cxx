#include "sys.h"
#include "debug.h"
#include "EdgeType.h"
#include "utils/is_power_of_two.h"
#include <iostream>

// This must match the order of EdgeType!
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
  return os << edge_str[edge_type];
}

std::ostream& operator<<(std::ostream& os, EdgeMaskType edge_mask_type)
{
  bool first = true;
  for (int shift = 0; shift < edge_bits; ++shift)
  {
    EdgeMaskTypePod bit = { uint32_t{1} << shift };
    if ((edge_mask_type & bit))
    {
      if (!first)
        os << '/';
      os << edge_str[shift];
      first = false;
    }
  }
  return os;
}

static char const* edge_color_array[edge_bits] =
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

char const* edge_color(EdgeType edge_type)
{
  return edge_color_array[edge_type];
}

static char const* edge_name_array[edge_bits] =
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

char const* edge_name(EdgeType edge_type)
{
  return edge_name_array[edge_type];
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START

channel_ct sb_edge("SB_EDGE");
channel_ct asw_edge("ASW_EDGE");
channel_ct dd_edge("DD_EDGE");
channel_ct cd_edge("CD_EDGE");
channel_ct rf_edge("RF_EDGE");
channel_ct tot_edge("TOT_EDGE");
channel_ct mo_edge("MO_EDGE");
channel_ct sc_edge("SC_EDGE");
channel_ct lo_edge("LO_EDGE");
channel_ct hb_edge("HB_EDGE");
channel_ct vse_edge("VSE_EDGE");
channel_ct vsses_edge("VSSES_EDGE");
channel_ct ithb_edge("ITHB_EDGE");
channel_ct dob_edge("DOB_EDGE");
channel_ct cad_edge("CAD_EDGE");
channel_ct sw_edge("SW_EDGE");
channel_ct hrs_edge("HRS_EDGE");
channel_ct rs_edge("RS_EDGE");
channel_ct dr_edge("DR_EDGE");
channel_ct ur_edge("UR_EDGE");

channel_ct* edge[edge_bits] =
  {
    &sb_edge,
    &asw_edge,
    &dd_edge,
    &cd_edge,
    &rf_edge,
    &tot_edge,
    &mo_edge,
    &sc_edge,
    &lo_edge,
    &hb_edge,
    &vse_edge,
    &vsses_edge,
    &ithb_edge,
    &dob_edge,
    &cad_edge,
    &sw_edge,
    &hrs_edge,
    &rs_edge,
    &dr_edge,
    &ur_edge
  };

NAMESPACE_DEBUG_CHANNELS_END
#endif
