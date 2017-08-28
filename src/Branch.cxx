#include "sys.h"
#include "Branch.h"
#include "Evaluation.h"
#include <ostream>

std::ostream& operator<<(std::ostream& os, Branch const& branch)
{
  os << "iff " << branch.m_conditional->second << " == " << (branch.m_conditional_true ? "true" : "false") << '}';
  return os;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct branch("BRANCH");
NAMESPACE_DEBUG_CHANNELS_END
#endif
