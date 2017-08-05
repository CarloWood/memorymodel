#include "sys.h"
#include "Node.h"
#include "debug_ostream_operators.h"
#include "utils/is_power_of_two.h"
#include "utils/macros.h"
#include "BooleanExpression.h"
#include <sstream>

char const* memory_order_str(std::memory_order memory_order)
{
  switch (memory_order)
  {
    case std::memory_order_relaxed:
      return "rlx";
    case std::memory_order_consume:
      return "con";
    case std::memory_order_acquire:
      return "acq";
    case std::memory_order_release:
      return "rel";
    case std::memory_order_acq_rel:   // acquire/release
      return "ar";
    case std::memory_order_seq_cst:   // sequentially consistent
      return "sc";
  }
  return "<UNKNOWN memory order>";
}

std::string NAReadNode::type() const
{
  return "Rna";
}

std::string AtomicReadNode::type() const
{
  std::string result = "R";
  result += memory_order_str(m_read_memory_order);
  return result;
}

std::string NAWriteNode::type() const
{
  return "Wna";
}

std::string AtomicWriteNode::type() const
{
  std::string result = "W";
  result += memory_order_str(m_write_memory_order);
  return result;
}

std::string MutexDeclNode::type() const
{
  return "Wna";
}

std::string MutexLockNode::type() const
{
  return "LK";
}

std::string MutexUnlockNode::type() const
{
  return "UL";
}

std::string RMWNode::type() const
{
  std::string result = "RMW";
  result += memory_order_str(m_memory_order);
  return result;
}

std::string CEWNode::type() const
{
  std::string result = "CEW";
  result += memory_order_str(m_write_memory_order);     // memory order of the RMW operation on success (m_variable == expected).
  result += ',';
  result += memory_order_str(m_fail_memory_order);
  return result;
}

std::string NodeBase::label(bool dot_file) const
{
  std::ostringstream ss;
  ss << name() << ':' << type() << ' ' << tag();
  print_code(ss, dot_file);
  return ss.str();
}

void NAReadNode::print_code(std::ostream& os, bool UNUSED_ARG(dot_file)) const
{
  //FIXME print what we read...
  //os << '<-';
}

void AtomicReadNode::print_code(std::ostream& os, bool UNUSED_ARG(dot_file)) const
{
  //FIXME print what we read...
  //os << '<-';
}

void WriteNode::print_code(std::ostream& os, bool dot_file) const
{
  os << '=';
  if (!dot_file)
    m_evaluation->print_on(os);
}

void MutexDeclNode::print_code(std::ostream& UNUSED_ARG(os), bool UNUSED_ARG(dot_file)) const
{
}

void MutexReadNode::print_code(std::ostream& UNUSED_ARG(os), bool UNUSED_ARG(dot_file)) const
{
}

void MutexLockNode::print_code(std::ostream& UNUSED_ARG(os), bool UNUSED_ARG(dot_file)) const
{
}

void MutexUnlockNode::print_code(std::ostream& UNUSED_ARG(os), bool UNUSED_ARG(dot_file)) const
{
}

void RMWNode::print_code(std::ostream& os, bool dot_file) const
{
  ASSERT(m_evaluation->binary_operator() == additive_ado_add || m_evaluation->binary_operator() == additive_ado_sub);
  os << ((m_evaluation->binary_operator() == additive_ado_add) ? "+=": "-=");
  if (!dot_file)
    m_evaluation->rhs()->print_on(os);
}

void CEWNode::print_code(std::ostream& os, bool dot_file) const
{
  os << "=" << m_expected << '?';
  if (!dot_file)
    os << m_desired;
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
  os << '{';
  Debug(os << edge.m_id << "; ");
  os << edge.m_edge_type << "; " << *edge.m_tail_node;
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

bool NodeBase::add_end_point(Edge* edge, EndPointType type, NodePtr const& other_node, bool edge_owner) const
{
  DoutEntering(dc::sb_edge, "NodeBase::add_end_point(" << *edge << ", " << type << ", " << *other_node << ", " << edge_owner << ") [this = " << *this << "]");
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

// A new incoming sequenced-before edge was added.
void NodeBase::update_exists() const
{
  DoutEntering(dc::sb_edge, "NodeBase::update_exists() [this = " << *this << "]");
  boolean::Expression node_exists(0);  // When does this node exist?
  int node_heads = 0;
  for (auto&& end_point : m_end_points)
  {
    if (end_point.edge_type() == edge_sb && end_point.type() == head)
    {
      ++node_heads;
      node_exists += end_point.edge()->exists();
      Dout(dc::sb_edge, "Found on this node (" << *this << ") an edge (from " << *end_point.other_node() <<
          ") which exists when " << end_point.edge()->exists() <<
          "; node_exists of this node is now " << node_exists);
    }
  }
  Dout(dc::sb_edge, "Setting m_exists to " << node_exists);
  m_exists = std::move(node_exists);
  for (auto&& end_point : m_end_points)
    if (end_point.edge_type() == edge_sb && end_point.type() == tail)
      end_point.other_node()->update_exists();
}

//static
bool NodeBase::add_edge(EdgeType edge_type, NodePtr const& tail_node, NodePtr const& head_node, Branches const& branches)
{
  DoutEntering(dc::sb_edge, "NodeBase::add_edge(" << edge_type << ", " << *tail_node << ", " << *head_node << ", " << branches << ")");
  Edge* new_edge = new Edge(edge_type, tail_node);
  new_edge->add_branches(branches);
  // For the sake of memory management, this EndPoint owns the allocated new_edge; so pass 'true'.
  // Call tail first!
  bool success2 = tail_node->add_end_point(new_edge, is_directed(edge_type) ? tail : undirected, head_node, false);
  bool success1 = head_node->add_end_point(new_edge, is_directed(edge_type) ? head : undirected, tail_node, true);
  ASSERT(success1 == success2);
  if (!success1) delete new_edge;
  if (success1)
  {
    Dout(dc::sb_edge, "ADDED EDGE " << *new_edge);
    if (edge_type == edge_sb)
      head_node->update_exists();
  }
  return success1;
}

//static
boolean::Expression const NodeBase::s_one(1);

boolean::Expression const& NodeBase::provides_sequenced_before_value_computation() const
{
  if (provided_type().type() == NodeProvidedType::side_effect)
    return m_connected.provides_sequenced_before_value_computation();
  return s_one;
}

boolean::Expression const& NodeBase::provides_sequenced_before_side_effect() const
{
  if (provided_type().type() == NodeProvidedType::value_computation)
    return m_connected.provides_sequenced_before_side_effect();
  return s_one;
}

// Called on the tail-node of a new (conditional) sb edge.
void NodeBase::sequenced_before() const
{
  using namespace boolean;
  DoutEntering(dc::sb_edge, "sequenced_before() [this = " << *this << "]");

  boolean::Expression sequenced_before_value_computation(0);
  boolean::Expression sequenced_before_side_effect(0);
  // Run over all outgoing (tail end_points) Sequenced-Before edges.
  for (auto&& end_point : m_end_points)
  {
    if (end_point.edge_type() == edge_sb && end_point.type() == tail)
    {
      Dout(dc::notice, "Found tail EndPoint " << end_point << " with condition '" << end_point.edge()->branches() << "'.");
      // Get condition of this edge.
      boolean::Product edge_conditional(end_point.edge()->branches().boolean_product());
      // Get the provides boolean expressions from the other node and AND them with the condition of that edge.
      // OR everything.
      sequenced_before_value_computation += end_point.other_node()->provides_sequenced_before_value_computation() * edge_conditional;
      sequenced_before_side_effect += end_point.other_node()->provides_sequenced_before_side_effect() * edge_conditional;
    }
    else
      Dout(dc::notice, "Skipping EndPoint " << end_point << '.');
  }
  Dout(dc::notice, "Result:");
  DebugMarkDownRight;
  Dout(dc::notice, "sequenced_before_value_computation = '" << sequenced_before_value_computation << "'.");
  Dout(dc::notice, "sequenced_before_side_effect = '" << sequenced_before_side_effect << "'.");

  // We don't support volatile memory accesses... otherwise a node could be a side_effect and value_computation at the same time :/
  bool node_provides_side_effect_not_value_computation = provided_type().type() == NodeProvidedType::side_effect;
  bool sequenced_before_value_computation_changed =
      m_connected.update_sequenced_before_value_computation(!node_provides_side_effect_not_value_computation, std::move(sequenced_before_value_computation));
  bool sequenced_before_side_effect_changed =
      m_connected.update_sequenced_before_side_effect(node_provides_side_effect_not_value_computation, std::move(sequenced_before_side_effect));

  if (sequenced_before_value_computation_changed || sequenced_before_side_effect_changed)
  {
    // Propagate bits to nodes sequenced before us.
    for (auto&& end_point : m_end_points)
      if (end_point.edge_type() == edge_sb && end_point.type() == head)
        end_point.other_node()->sequenced_before();
  }
}

bool NodeBase::provides_sequenced_after_something() const
{
  bool result = true;
  if (provided_type().type() == NodeProvidedType::side_effect)
    result = m_connected.provides_sequenced_after_something();
  return result;
}

// Called on the head-node of a new (conditional) sb edge.
void NodeBase::sequenced_after() const
{
  DoutEntering(dc::sb_edge, "sequenced_after() [this = " << *this << "]");
  m_connected.set_sequenced_after_something();
}

// This is a value-computation node that no longer should be marked as value-computation
// head, because it is sequenced before a side-effect that must be sequenced before
// it's value computation (which then becomes the new value-computation head).
void NodeBase::sequenced_before_side_effect_sequenced_before_value_computation() const
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
  m_connected.update_sequenced_before_value_computation(true, boolean::Expression::one());
#ifdef CWDEBUG
  for (auto&& end_point : m_end_points)
    if (end_point.edge_type() == edge_sb && end_point.type() == head)
    {
      Dout(dc::debug, "Node " << *this << " has head end_point " << end_point << '!');
      // If the found head isn't already marked as being sequenced before a value computation
      // then we need to update it! I don't expect this to ever happen.
      ASSERT(end_point.other_node()->m_connected.provides_sequenced_before_value_computation().is_one());
    }
#endif
}

// This is the write node of a prefix operator that read from read_node,
// or the write node of an assignment expression whose value computation
// is used.
void NodeBase::sequenced_before_value_computation() const
{
  Dout(dc::notice, "Marking " << *this << " as sequenced before its value computation.");
  m_connected.set_sequenced_before_pseudo_value_computation();
}

bool NodeBase::matches(NodeRequestedType const& requested_type, boolean::Expression& hiding) const
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

  hiding = m_connected.hiding_behind_another(requested_type);
  if (hiding.is_one())
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
