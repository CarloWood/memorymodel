#include "sys.h"
#include "Node.h"
#include "utils/is_power_of_two.h"
#include "utils/macros.h"
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
