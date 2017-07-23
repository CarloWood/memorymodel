#include "sys.h"
#include "Node.h"
#include "debug_ostream_operators.h"
#include "utils/is_power_of_two.h"
#include "utils/macros.h"
#include "boolexpr/boolexpr.h"
#include <sstream>

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

char const* sb_mask_str(Node::filter_type bit)
{
  ASSERT(utils::is_power_of_two(bit));
  switch (bit)
  {
    AI_CASE_RETURN(Node::value_computation_tails);
    AI_CASE_RETURN(Node::value_computation_heads);
    AI_CASE_RETURN(Node::side_effect_tails);
    AI_CASE_RETURN(Node::side_effect_heads);
    AI_CASE_RETURN(Node::sequenced_before_pseudo_value_computation_bit);
    AI_CASE_RETURN(Node::anything);
  }
  return "UNKNOWN filter_type";
}

std::ostream& operator<<(std::ostream& os, Node::Filter filter)
{
  Node::filter_type sb_mask = filter.m_connected;
  if (sb_mask == Node::heads)
    os << "Node::heads";
  else if (sb_mask == Node::tails)
    os << "Node::tails";
  else
  {
    bool first = true;
    for (Node::filter_type bit = 1; bit != Node::sb_unused_bit; bit <<= 1)
    {
      if ((sb_mask & bit))
      {
        if (!first)
          os << '|';
        os << sb_mask_str(bit);
        first = false;
      }
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

std::ostream& operator<<(std::ostream& os, Edge const& edge)
{
  os << '{' << edge.m_edge_type;
  if (edge.m_branches.conditional())
    os << "; " << edge.m_branches;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, EndPoint const& end_point)
{
  os << '{' << *end_point.m_edge << ", " << end_point.m_type << ", " << *end_point.m_other_node << '}';
  return os;
}

bool operator==(EndPoint const& end_point1, EndPoint const& end_point2)
{
  return *end_point1.m_edge == *end_point2.m_edge &&
         end_point1.m_type == end_point2.m_type &&
         *end_point1.m_other_node == *end_point2.m_other_node;
}

bool Node::add_end_point(Edge* edge, EndPointType type, EndPoint::node_iterator const& other_node, bool edge_owner) const
{
  m_end_points.emplace_back(edge, type, other_node, edge_owner);
  end_points_type::iterator begin = m_end_points.begin();
  end_points_type::iterator last = m_end_points.end();
  end_points_type::iterator iter = --last;      // Point to just added element.
  while (iter != begin)
    if (AI_UNLIKELY(*last == *--iter))          // Is added end point equal to already existing one?
    {
      // End point already existed.
      m_end_points.pop_back();
      return false;
    }
  // End point did not already exist.
  return true;
}

//static
bool Node::add_edge(EdgeType edge_type, EndPoint::node_iterator const& tail_node, EndPoint::node_iterator const& head_node, Branches const& branches)
{
  Edge* new_edge = new Edge(edge_type);
  new_edge->add_branches(branches);
  // For the sake of memory management, this EndPoint owns the allocated new_edge; so pass 'true'.
  bool success1 = head_node->add_end_point(new_edge, is_directed(edge_type) ? head : undirected, tail_node, true);
  bool success2 = tail_node->add_end_point(new_edge, is_directed(edge_type) ? tail : undirected, head_node, false);
  ASSERT(success1 == success2);
  if (!success1) delete new_edge;
  return success1;
}

// Called on the tail-node of a new edge.
void Node::sequenced_before(Node const& head_node) const
{
  DoutEntering(dc::sb_edge, "sequenced_before(" << head_node << ") [this = " << *this << "]");
  filter_type orig = m_connected;
  m_connected |= (head_node.provided_type() | head_node.m_connected) & sequenced_before_mask;
  Dout(dc::sb_edge, "m_connected changed from " << Filter(orig) << " to " << Filter(m_connected) << ".");
  // Propegation might be needed.
  // For example,
  //                flags
  //     a:W        sequenced_before_value_computation_bit
  //      |
  //      v
  //     b:R        sequenced_after_side_effect_bit
  //      .
  //      .
  //     c:W
  //
  // When b.sequenced_before(c) is called, the mask of b is updated from sequenced_after_side_effect_bit to
  // sequenced_after_side_effect_bit|sequenced_before_side_effect_bit, and since it itself is not a side-effect
  // it is needed to propagate the sequenced_before_side_effect_bit upwards to a.
  //
  filter_type set_bits = m_connected & ~orig;          // Did any bit(s) get set?
  if ((set_bits & ~provided_type()))                 // Are we not already of that type?
  {
    // Propagate bits to nodes sequenced before us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == head)
        end_point.other_node()->sequenced_before(head_node);
  }
}

// Called on the head-node of a new edge.
void Node::sequenced_after(Node const& tail_node) const
{
  DoutEntering(dc::sb_edge, "sequenced_after(" << tail_node << ") [this = " << *this << "]");
  filter_type orig = m_connected;
  m_connected |= (tail_node.provided_type() | tail_node.m_connected) & sequenced_after_mask;
  Dout(dc::sb_edge, "m_connected changed from " << Filter(orig) << " to " << Filter(m_connected) << ".");
  // Same as above.
  filter_type set_bits = m_connected & ~orig;
  if ((set_bits & ~provided_type()))
  {
    // Propagate bits to nodes sequenced after us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == tail)
        end_point.other_node()->sequenced_after(tail_node);
  }
}

// This is a value-computation node that no longer should be marked as head,
// because it is sequenced before a side-effect that must be sequenced before
// it's value computation (which then becomes the new head).
void Node::sequenced_before_side_effect_sequenced_before_value_computation() const
{
  Dout(dc::notice, "Marking " << *this << " as sequenced before a (pseudo) value computation.");
  m_connected |= sequenced_before_value_computation_bit;
}

// This is the write node of a prefix operator that read from read_node,
// or the write node of an assignment expression whose value computation
// is used.
void Node::sequenced_before_value_computation() const
{
  Dout(dc::notice, "Marking " << *this << " as sequenced before its value computation.");
  m_connected |= sequenced_before_pseudo_value_computation_bit;
}

// Returns true when this Node is of the requested_type type (value-computation or side-effect)
// and is a head or tail (as requested_type) for that type.
//
// For example,
//                  flags
//     a:W          sequenced_before_value_computation_bit|sequenced_before_side_effect_bit
//      |
//      v
//     b:W          sequenced_before_value_computation_bit|sequenced_after_side_effect_bit
//      |
//      v
//     c:R          sequenced_before_value_computation_bit|sequenced_after_side_effect_bit
//      |
//      v
//     d:R           sequenced_after_value_computation_bit|sequenced_after_side_effect_bit
//
// should return
//
// head_tail_mask             a:      b:      c:      d:
//
// value_computation_tails    false   false   true    false
// value_computation_heads    false   false   false   true
// side_effect_tails          true    false   false   false
// side_effect_heads          false   true    false   false
// tails                      true    false   false   false
// heads                      false   false   false   true
//
bool Node::is_filtered(filter_type requested_type) const
{
  if (requested_type == anything)
    return true;

  DoutEntering(dc::sb_edge, "is_filtered(" << Filter(requested_type) << ") [this = " << *this << "]");
  Dout(dc::sb_edge, "provided_type() = " << Filter(provided_type()) << "; m_connected = " << Filter(m_connected));

  // Is itself the requested_type type (value-computation or side-effect)?
  bool is_requested_type = (provided_type() & requested_type) ||
      (m_connected & sequenced_before_pseudo_value_computation_bit);   // The write node of a pre-increment/decrement expression fakes
                                                                       // being both, value-computation and side-effect.
  if (!is_requested_type)
    Dout(dc::sb_edge, "rejected because we ourselves are not the requested_type evaluation type.");
  // Are we hiding behind another node of that type?
  // For example, if we're looking for a head, then B hides behind C (which is the real head): A--->B--->C
  bool hiding_behind_another = m_connected & requested_type;
  if (hiding_behind_another)
    Dout(dc::sb_edge, "rejected because we are hiding behind another node that provides the requested_type type.");
  return is_requested_type && !hiding_behind_another;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_edge("SB_EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
