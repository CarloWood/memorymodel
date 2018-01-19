#include "sys.h"
#include "ReleaseSequences.h"

std::ostream& operator<<(std::ostream& os, RSIndex index)
{
  if (index.undefined())
    os << "RS?";
  else
    os << "RS" << index.get_value();
  return os;
}

RSIndex ReleaseSequences::find(SequenceNumber rs_begin, SequenceNumber rs_end) const
{
  for (RSIndex index = m_release_sequences.ibegin(); index != m_release_sequences.iend(); ++index)
    if (m_release_sequences[index].equals(rs_begin, rs_end))
      return index;
  return RSIndex();
}

ReleaseSequence const& ReleaseSequences::insert(SequenceNumber rs_begin, SequenceNumber rs_end)
{
  RSIndex index = m_release_sequences.ibegin();
  while (index != m_release_sequences.iend())
  {
    if (m_release_sequences[index].equals(rs_begin, rs_end))
      return m_release_sequences[index];
    ++index;
  }
  m_release_sequences.emplace_back(rs_begin, rs_end);
  return m_release_sequences.back();
}
