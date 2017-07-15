#include "sys.h"
#include "debug.h"
#include "Edge.h"
#include "Node.h"
#include "utils/macros.h"
#include <iostream>

char const* edge_str(edge_type type)
{
  switch (type)
  {
    case edge_sb: return "Sequenced-Before";
    case edge_asw: return "Additional-Synchronises-With";
    case edge_dd: return "Data-Dependency";
    case edge_cd: return "Control-Dependency";
    AI_CASE_RETURN(edge_rf);
    AI_CASE_RETURN(edge_tot);
    AI_CASE_RETURN(edge_mo);
    AI_CASE_RETURN(edge_sc);
    AI_CASE_RETURN(edge_lo);
    AI_CASE_RETURN(edge_hb);
    AI_CASE_RETURN(edge_vse);
    AI_CASE_RETURN(edge_vsses);
    AI_CASE_RETURN(edge_ithb);
    AI_CASE_RETURN(edge_dob);
    AI_CASE_RETURN(edge_cad);
    AI_CASE_RETURN(edge_sw);
    AI_CASE_RETURN(edge_hrs);
    AI_CASE_RETURN(edge_rs);
    AI_CASE_RETURN(edge_data_races);
    AI_CASE_RETURN(edge_unsequenced_races);
  }
  return "UNKNOWN edge_type";
}

std::ostream& operator<<(std::ostream& os, edge_type type)
{
  return os << edge_str(type);
}

bool operator<(Edge const& edge1, Edge const& edge2)
{
  // An edge is different from a node: when a new Node is inserted,
  // it is guaranteed to be a new Node so we can sort them by id.
  // But a request for a new edge is less certain (though it might
  // be the case that they are always new). Therefore, compare
  // Edges based on what matters for equality (everything EXCEPT
  // the id).
  return edge1.m_type  <  edge2.m_type   || (!( edge2.m_type  <  edge1.m_type ) && (
        *edge1.m_begin < *edge2.m_begin  || (!(*edge2.m_begin < *edge1.m_begin) &&
        *edge1.m_end   < *edge2.m_end)));
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_edge("SB_EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
