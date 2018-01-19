#pragma once

#include "ReleaseSequence.h"
#include "utils/Vector.h"

namespace ordering_category {
struct RSIndex;         // All ReleaseSequences.
} // namespace ordering_category

using RSIndex = utils::VectorIndex<ordering_category::RSIndex>;
using RSIndexOrderedReleaseSequences = utils::Vector<ReleaseSequence, RSIndex>;

std::ostream& operator<<(std::ostream& os, RSIndex index);

class ReleaseSequences
{
 private:
  RSIndexOrderedReleaseSequences m_release_sequences;

 public:
  RSIndex find(SequenceNumber rs_begin, SequenceNumber rs_end) const;
  ReleaseSequence const& insert(SequenceNumber rs_begin, SequenceNumber rs_end);
  ReleaseSequence const& operator[](RSIndex index) const { return m_release_sequences[index]; }
  RSIndex ibegin() const { return m_release_sequences.ibegin(); }
  RSIndex iend() const { return m_release_sequences.iend(); }
};
