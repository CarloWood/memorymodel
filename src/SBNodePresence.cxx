#include "sys.h"
#include "debug.h"
#include "SBNodePresence.h"
#include <ostream>

#if 0
  NodeProvidedType::value_computation
  NodeProvidedType::side_effect

  value_computation_heads_
  heads_
  tails_

  static int const sequenced_after_value_computation_         = 0;       // We are sequenced after a value-computation Node.
  static int const sequenced_before_value_computation_        = 1;       // We are sequenced before a value-computation Node.
  static int const sequenced_after_side_effect_               = 2;       // We are sequenced after a side-effect Node.
  static int const sequenced_before_side_effect_              = 3;       // We are sequenced before a side-effect Node.
#endif

bool SBNodePresence::update_sequenced_before_value_computation(bool node_provides, boolexpr::bx_t sequenced_before_value_computation)
{
  bool changed = !(*m_bxs)[sequenced_before_value_computation_]->equiv(sequenced_before_value_computation);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_before_value_computation] changed from " <<
        (*m_bxs)[sequenced_before_value_computation_]->to_string() <<
        " to " << sequenced_before_value_computation->to_string() << ".");
    (*m_bxs)[sequenced_before_value_computation_] = sequenced_before_value_computation;
  }
  return !node_provides && changed;
}

bool SBNodePresence::update_sequenced_before_side_effect(bool node_provides, boolexpr::bx_t sequenced_before_side_effect)
{
  bool changed = !(*m_bxs)[sequenced_before_side_effect_]->equiv(sequenced_before_side_effect);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_before_side_effect] changed from " <<
        (*m_bxs)[sequenced_before_side_effect_]->to_string() <<
        " to " << sequenced_before_side_effect->to_string() << ".");
    (*m_bxs)[sequenced_before_side_effect_] = sequenced_before_side_effect;
  }
  return !node_provides && changed;
}

bool SBNodePresence::update_sequenced_after_value_computation(bool node_provides, boolexpr::bx_t sequenced_after_value_computation)
{
  bool changed = !(*m_bxs)[sequenced_after_value_computation_]->equiv(sequenced_after_value_computation);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_after_value_computation] changed from " <<
        (*m_bxs)[sequenced_after_value_computation_]->to_string() <<
        " to " << sequenced_after_value_computation->to_string() << ".");
    (*m_bxs)[sequenced_after_value_computation_] = sequenced_after_value_computation;
  }
  return !node_provides && changed;
}

bool SBNodePresence::update_sequenced_after_side_effect(bool node_provides, boolexpr::bx_t sequenced_after_side_effect)
{
  bool changed = !(*m_bxs)[sequenced_after_side_effect_]->equiv(sequenced_after_side_effect);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_after_side_effect] changed from " <<
        (*m_bxs)[sequenced_after_side_effect_]->to_string() <<
        " to " << sequenced_after_side_effect->to_string() << ".");
    (*m_bxs)[sequenced_after_side_effect_] = sequenced_after_side_effect;
  }
  return !node_provides && changed;
}

boolexpr::bx_t SBNodePresence::hiding_behind_another(NodeRequestedType const& requested_type)
{
  switch (requested_type.type())
  {
    case NodeRequestedType::value_computation_heads_:
      return (*m_bxs)[sequenced_before_value_computation_];
    case NodeRequestedType::heads_:
      return boolexpr::or_s({(*m_bxs)[sequenced_before_value_computation_], (*m_bxs)[sequenced_before_side_effect_]});
    case NodeRequestedType::tails_:
      return boolexpr::or_s({(*m_bxs)[sequenced_after_value_computation_], (*m_bxs)[sequenced_after_side_effect_]});
    case NodeRequestedType::all_:
      ;
  }
  // Avoid compiler warning...
  return boolexpr::zero();
}

bool NodeRequestedType::matches(NodeProvidedType const& provided_type) const
{
  // The requested type always matches the provided type unless we
  // request a value_computation (head) and a side effect is provided.
  return provided_type.type() != NodeProvidedType::side_effect || m_type != value_computation_heads_;
}

std::ostream& operator<<(std::ostream& os, NodeProvidedType const& provided_type)
{
  switch (provided_type.m_type)
  {
    case NodeProvidedType::value_computation:
      os << "value_computation";
      break;
    case NodeProvidedType::side_effect:
      os << "side_effect";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, NodeRequestedType const& requested_type)
{
  switch (requested_type.m_type)
  {
    case NodeRequestedType::value_computation_heads_:
      os << "value_computation_heads";
      break;
    case NodeRequestedType::heads_:
      os << "heads";
      break;
    case NodeRequestedType::tails_:
      os << "tails";
      break;
    case NodeRequestedType::all_:
      os << "all";
      break;
  }
  return os;
}

void SBNodePresence::print_on(std::ostream& os) const
{
  bool first = true;
  for (int i = 0; i < array_size; ++i)
  {
    if (!IS_ZERO((*m_bxs)[i]))
    {
      if (!first)
        os << ", ";
      os << s_name[i] << ':';
      os << (*m_bxs)[i]->to_string();
      first = false;
    }
  }
  if (m_sequenced_before_pseudo_value_computation)
  {
    if (!first)
      os << ", ";
    os << "m_sequenced_before_pseudo_value_computation: 1";
    first = false;
  }
  if (first)
    os << "Nothing";
}

//static
char const* const SBNodePresence::s_name[SBNodePresence::array_size] = {
  "sequenced_after_value_computation",
  "sequenced_before_value_computation",
  "sequenced_after_side_effect",
  "sequenced_before_side_effect",
};

//static
NodeRequestedType const NodeRequestedType::value_computation_heads(NodeRequestedType::value_computation_heads_);
NodeRequestedType const NodeRequestedType::heads(NodeRequestedType::heads_);
NodeRequestedType const NodeRequestedType::tails(NodeRequestedType::tails_);
NodeRequestedType const NodeRequestedType::all(NodeRequestedType::all_);

//static
boolexpr::zero_t SBNodePresence::s_zero{boolexpr::zero()};
