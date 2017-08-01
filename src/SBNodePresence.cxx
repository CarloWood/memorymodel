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

  static int const sequenced_before_value_computation_        = 0;       // We are sequenced before a value-computation Node.
  static int const sequenced_before_side_effect_              = 1;       // We are sequenced before a side-effect Node.
#endif

bool SBNodePresence::update_sequenced_before_value_computation(bool node_provides, boolean::Expression&& sequenced_before_value_computation)
{
  bool changed = !m_sequenced_before_value_computation.equivalent(sequenced_before_value_computation);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_before_value_computation] changed from " <<
        m_sequenced_before_value_computation << " to " << sequenced_before_value_computation << ".");
    m_sequenced_before_value_computation = std::move(sequenced_before_value_computation);
  }
  return !node_provides && changed;
}

bool SBNodePresence::update_sequenced_before_side_effect(bool node_provides, boolean::Expression&& sequenced_before_side_effect)
{
  bool changed = !m_sequenced_before_side_effect.equivalent(sequenced_before_side_effect);
  if (changed)
  {
    Dout(dc::sb_edge, "m_connected[sequenced_before_side_effect] changed from " <<
        m_sequenced_before_side_effect << " to " << sequenced_before_side_effect << ".");
    m_sequenced_before_side_effect = std::move(sequenced_before_side_effect);
  }
  return !node_provides && changed;
}

void SBNodePresence::set_sequenced_after_something()
{
  if (!m_sequenced_after_something)
    Dout(dc::sb_edge, "m_connected[sequenced_after_something] set to 1.");
  m_sequenced_after_something = true;
}

boolean::Expression SBNodePresence::hiding_behind_another(NodeRequestedType const& requested_type)
{
  switch (requested_type.type())
  {
    case NodeRequestedType::value_computation_heads_:
      return m_sequenced_before_value_computation.copy();
    case NodeRequestedType::heads_:
      return m_sequenced_before_value_computation + m_sequenced_before_side_effect;
    case NodeRequestedType::tails_:
      if (m_sequenced_after_something)
        return boolean::Expression::one();
      /*fall-through*/
    case NodeRequestedType::all_: ;     // Avoid compiler warning.
      /*fall-through*/
  }
  return boolean::Expression::zero();
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
  if (!m_sequenced_before_value_computation.is_zero())
  {
    if (!first)
      os << ", ";
    os << "m_sequenced_before_value_computation" << ':' << m_sequenced_before_value_computation;
    first = false;
  }
  if (!m_sequenced_before_side_effect.is_zero())
  {
    if (!first)
      os << ", ";
    os << "m_sequenced_before_side_effect" << ':' << m_sequenced_before_side_effect;
    first = false;
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
NodeRequestedType const NodeRequestedType::value_computation_heads(NodeRequestedType::value_computation_heads_);
NodeRequestedType const NodeRequestedType::heads(NodeRequestedType::heads_);
NodeRequestedType const NodeRequestedType::tails(NodeRequestedType::tails_);
NodeRequestedType const NodeRequestedType::all(NodeRequestedType::all_);
