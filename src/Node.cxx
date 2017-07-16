#include "sys.h"
#include "Node.h"
#include "utils/is_power_of_two.h"
#include "utils/macros.h"
#include <sstream>

bool operator==(EndPoint const& end_point1, EndPoint const& end_point2)
{
  return end_point1.m_edge_type == end_point2.m_edge_type &&
         end_point1.m_type == end_point2.m_type &&
         *end_point1.m_other_node == *end_point2.m_other_node;
}

std::string Node::type() const
{
  std::string type;
  switch (m_access)
  {
    case ReadAccess:
    case MutexLock1:
    case MutexUnlock1:
      type = "R";
      break;
    case WriteAccess:
    case MutexDeclaration:
      type = "W";
      break;
    case MutexLock2:
      return "LK";
    case MutexUnlock2:
      return "UL";
  }
  if (!m_atomic)
    type += "na";
  else
    switch (m_memory_order)
    {
      case std::memory_order_relaxed:   // relaxed
        type += "rlx";
        break;
      case std::memory_order_consume:   // consume
        type += "con";
        break;
      case std::memory_order_acquire:   // acquire
        type += "acq";
        break;
      case std::memory_order_release:   // release
        type += "rel";
        break;
      case std::memory_order_acq_rel:   // acquire/release
        // This should not happen because this is always either a Read or Write
        // operation and therefore either acquire or release respectively.
        DoutFatal(dc::core, "memory_order_acq_rel for Node type?!");
        break;
      case std::memory_order_seq_cst:   // sequentially consistent
        type += "sc";
        break;
    }

  return type;
}

std::string Node::label(bool dot_file) const
{
  std::ostringstream ss;
  ss << name() << ':' << type() << ' ' << tag() << '=';
  if (!dot_file)
  {
    if (is_write())
      m_evaluation->print_on(ss);
  }
  return ss.str();
}

char const* sb_mask_str(Node::sb_mask_type bit)
{
  ASSERT(utils::is_power_of_two(bit));
  switch (bit)
  {
    AI_CASE_RETURN(Node::value_computation_tails);
    AI_CASE_RETURN(Node::value_computation_heads);
    AI_CASE_RETURN(Node::side_effect_tails);
    AI_CASE_RETURN(Node::side_effect_heads);
  }
  return "UNKNOWN sb_mask_type";
}

std::ostream& operator<<(std::ostream& os, Node::Filter filter)
{
  Node::sb_mask_type sb_mask = filter.m_sb_mask;
  if (sb_mask == Node::heads)
    os << "Node::heads";
  else if (sb_mask == Node::tails)
    os << "Node::tails";
  else
  {
    bool first = true;
    for (Node::sb_mask_type bit = 1; bit != Node::sb_unused_bit; bit <<= 1)
    {
      if ((sb_mask & bit))
      {
        if (!first)
          os << '|';
        os << sb_mask_str(bit);
      }
      first = false;
    }
  }
  return os;
}

char const* edge_str(EdgeType edge_type)
{
  switch (edge_type)
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

std::ostream& operator<<(std::ostream& os, EdgeType edge_type)
{
  return os << edge_str(edge_type);
}

char const* end_point_str(EndPointType end_point_type)
{
  switch (end_point_type)
  {
    AI_CASE_RETURN(undirected);
    AI_CASE_RETURN(tail);
    AI_CASE_RETURN(head);
  }
  return "UNKNOWN EndPointType";
}

std::ostream& operator<<(std::ostream& os, EndPointType end_point_type)
{
  return os << end_point_str(end_point_type);
}

std::ostream& operator<<(std::ostream& os, EndPoint const& end_point)
{
  os << '{' << end_point.m_edge_type << ", " << end_point.m_type << ", " << *end_point.m_other_node << '}';
  return os;
}

void Node::add_end_point(EdgeType edge_type, EndPointType type, EndPoint::node_iterator const& other_node) const
{
  ASSERT(std::find(m_end_points.begin(), m_end_points.end(), EndPoint(edge_type, type, other_node)) == m_end_points.end());
  m_end_points.emplace_back(edge_type, type, other_node);
}

//static
void Node::add_edge(EdgeType edge_type, EndPoint::node_iterator const& tail_node, EndPoint::node_iterator const& head_node)
{
  head_node->add_end_point(edge_type, is_directed(edge_type) ? head : undirected, tail_node);
  tail_node->add_end_point(edge_type, is_directed(edge_type) ? tail : undirected, head_node);
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_edge("SB_EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
