#include "sys.h"
#include "ReleaseSequence.h"
#include <ostream>

std::string ReleaseSequence::id_name() const
{
  return std::string(1, 'A' + m_id);
}

std::ostream& operator<<(std::ostream& os, ReleaseSequence const& release_sequence)
{
  os << "{" << release_sequence.m_begin << "-rs:" << release_sequence.m_boolexpr_variable << "->" << release_sequence.m_end << " [" << release_sequence.m_id << "]}";
  return os;
}

//static
ReleaseSequence::id_type ReleaseSequence::s_next_release_sequence_id;
