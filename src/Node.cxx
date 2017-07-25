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

boolexpr::bx_t Node::provides_sequenced_before_value_computation() const
{
  boolexpr::bx_t result{boolexpr::one()};
  if (provided_type().type() == NodeProvidedType::side_effect)
    result = m_connected.provides_sequenced_before_value_computation();
  return result;
}

boolexpr::bx_t Node::provides_sequenced_before_side_effect() const
{
  boolexpr::bx_t result{boolexpr::one()};
  if (provided_type().type() == NodeProvidedType::value_computation)
    result = m_connected.provides_sequenced_before_side_effect();
  return result;
}

// Called on the tail-node of a new (conditional) sb edge.
void Node::sequenced_before() const
{
  using namespace boolexpr;
  DoutEntering(dc::sb_edge, "sequenced_before() [this = " << *this << "]");

  bx_t sequenced_before_value_computation{zero()};
  bx_t sequenced_before_side_effect{zero()};
  // Run over all outgoing (tail end_points) Sequenced-Before edges.
  for (auto&& end_point : m_end_points)
  {
    if (end_point.edge_type() == edge_sb && end_point.type() == tail)
    {
      Dout(dc::notice, "Found tail EndPoint " << end_point << " with condition '" << end_point.edge()->branches() << "'.");
      // Get condition of this edge.
      bx_t edge_bx{end_point.edge()->branches().boolean_expression()};
      // Get the provides boolean expressions from the other node and AND them with the condition of that edge.
      // OR everything.
      sequenced_before_value_computation = sequenced_before_value_computation | (edge_bx & end_point.other_node()->provides_sequenced_before_value_computation());
      sequenced_before_side_effect = sequenced_before_side_effect | (edge_bx & end_point.other_node()->provides_sequenced_before_side_effect());
    }
    else
      Dout(dc::notice, "Skipping EndPoint " << end_point << '.');
  }
  sequenced_before_value_computation = sequenced_before_value_computation->simplify();
  sequenced_before_side_effect = sequenced_before_side_effect->simplify();
  Dout(dc::notice, "Result:");
  DebugMarkDownRight;
  Dout(dc::notice, "sequenced_before_value_computation = '" << sequenced_before_value_computation << "'.");
  Dout(dc::notice, "sequenced_before_side_effect = '" << sequenced_before_side_effect << "'.");

  // We don't support volatile memory accesses... otherwise a node could be a side_effect and value_computation at the same time :/
  bool node_provides_side_effect_not_value_computation = provided_type().type() == NodeProvidedType::side_effect;
  bool sequenced_before_value_computation_changed =
      m_connected.update_sequenced_before_value_computation(!node_provides_side_effect_not_value_computation, sequenced_before_value_computation);
  bool sequenced_before_side_effect_changed =
      m_connected.update_sequenced_before_side_effect(node_provides_side_effect_not_value_computation, sequenced_before_side_effect);

  if (sequenced_before_value_computation_changed || sequenced_before_side_effect_changed)
  {
    // Propagate bits to nodes sequenced before us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == head)
        end_point.other_node()->sequenced_before();
  }
}

boolexpr::bx_t Node::provides_sequenced_after_value_computation() const
{
  boolexpr::bx_t result{boolexpr::one()};
  if (provided_type().type() == NodeProvidedType::side_effect)
    result = m_connected.provides_sequenced_after_value_computation();
  return result;
}

boolexpr::bx_t Node::provides_sequenced_after_side_effect() const
{
  boolexpr::bx_t result{boolexpr::one()};
  if (provided_type().type() == NodeProvidedType::value_computation)
    result = m_connected.provides_sequenced_after_side_effect();
  return result;
}

// Called on the head-node of a new (conditional) sb edge.
void Node::sequenced_after() const
{
  using namespace boolexpr;
  DoutEntering(dc::sb_edge, "sequenced_after() [this = " << *this << "]");

  bx_t sequenced_after_value_computation{zero()};
  bx_t sequenced_after_side_effect{zero()};
  // Run over all outgoing (head end_points) Sequenced-Before edges.
  for (auto&& end_point : m_end_points)
  {
    if (end_point.edge_type() == edge_sb && end_point.type() == head)
    {
      Dout(dc::notice, "Found head EndPoint " << end_point << " with condition '" << end_point.edge()->branches() << "'.");
      // Get condition of this edge.
      bx_t edge_bx{end_point.edge()->branches().boolean_expression()};
      // Get the provides boolean expressions from the other node and AND them with the condition of that edge.
      // OR everything.
      sequenced_after_value_computation = sequenced_after_value_computation | (edge_bx & end_point.other_node()->provides_sequenced_after_value_computation());
      sequenced_after_side_effect = sequenced_after_side_effect | (edge_bx & end_point.other_node()->provides_sequenced_after_side_effect());
    }
    else
      Dout(dc::notice, "Skipping EndPoint " << end_point << '.');
  }
  sequenced_after_value_computation = sequenced_after_value_computation->simplify();
  sequenced_after_side_effect = sequenced_after_side_effect->simplify();
  Dout(dc::notice, "Result:");
  DebugMarkDownRight;
  Dout(dc::notice, "sequenced_after_value_computation = '" << sequenced_after_value_computation << "'.");
  Dout(dc::notice, "sequenced_after_side_effect = '" << sequenced_after_side_effect << "'.");

  // We don't support volatile memory accesses... otherwise a node could be a side_effect and value_computation at the same time :/
  bool node_provides_side_effect_not_value_computation = provided_type().type() == NodeProvidedType::side_effect;
  bool sequenced_after_value_computation_changed =
      m_connected.update_sequenced_after_value_computation(!node_provides_side_effect_not_value_computation, sequenced_after_value_computation);
  bool sequenced_after_side_effect_changed =
      m_connected.update_sequenced_after_side_effect(node_provides_side_effect_not_value_computation, sequenced_after_side_effect);

  if (sequenced_after_value_computation_changed || sequenced_after_side_effect_changed)
  {
    // Propagate bits to nodes sequenced after us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == tail)
        end_point.other_node()->sequenced_after();
  }
}

// This is a value-computation node that no longer should be marked as value-computation
// head, because it is sequenced before a side-effect that must be sequenced before
// it's value computation (which then becomes the new value-computation head).
void Node::sequenced_before_side_effect_sequenced_before_value_computation() const
{
  Dout(dc::notice, "Marking " << *this << " as sequenced before a (pseudo) value computation.");
  // Passing one() as boolean expression because we are unconditionally sequenced before
  // a related side-effect that is unconditionally sequenced before the value computation
  // of the expression that this node is currently a tail of.
  //
  // For example, the expression ++x has a Read node of x sequenced before
  // the Write node that writes to x:
  //
  //     Rna x=
  //       |
  //       v
  //     Wna x=
  //       |
  //       v
  //     Pseudo value computation Node
  //
  // Both edges have no condition of their own, and we are not linked to
  // any other node yet, so their condition is 1.
  //
  // Bottom line, we NEVER want to add a tail to the "Rna x=".
  m_connected.update_sequenced_before_value_computation(true, boolexpr::one());
#ifdef CWDEBUG
  for (auto&& end_point : m_end_points)
    if (end_point.edge_type() == edge_sb && end_point.type() == head)
    {
      Dout(dc::debug, "Node " << *this << " has head end_point " << end_point << '!');
      // If the found head isn't already marked as being sequenced before a value computation
      // then we need to update it! I don't expect this to ever happen.
      ASSERT(IS_ONE(end_point.other_node()->m_connected.provides_sequenced_before_value_computation()));
    }
#endif
}

// This is the write node of a prefix operator that read from read_node,
// or the write node of an assignment expression whose value computation
// is used.
void Node::sequenced_before_value_computation() const
{
  Dout(dc::notice, "Marking " << *this << " as sequenced before its value computation.");
  m_connected.set_sequenced_before_pseudo_value_computation();
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
bool Node::matches(NodeRequestedType const& requested_type, boolexpr::bx_t& matches) const
{
  if (requested_type.any())
    return true;

  DoutEntering(dc::sb_edge, "matches(" << requested_type << ") [this = " << *this << "]");
  Dout(dc::sb_edge, "provided_type() = " << provided_type() << "; m_connected = " << m_connected);

  // Is itself the requested_type type (value-computation or side-effect)?
  bool is_requested_type =
      m_connected.sequenced_before_pseudo_value_computation() ||        // The write node of a pre-increment/decrement expression
      requested_type.matches(provided_type());                          //   fakes being both, value-computation and side-effect.

  if (!is_requested_type)
  {
    Dout(dc::sb_edge, "rejected because we ourselves are not the requested_type evaluation type.");
    return false;
  }

  matches = ~m_connected.hiding_behind_another(requested_type);
  if (IS_ZERO(matches))
  {
    Dout(dc::sb_edge, "rejected because we are hiding behind another node that provides the requested_type type.");
    return false;
  }

  return true;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct sb_edge("SB_EDGE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
