#pragma once

#include "boolean-expression/BooleanExpression.h"
#include "TopologicalOrderedActions.h"
#include "debug.h"
#include <string>

struct ReleaseSequence
{
  using id_type = int;

  // A ReleaseSequence is uniquely defined by the (opsem) nodes where
  // the release sequence begins (the Write release) and ends (the relaxed
  // Write whose value is read by the Read acquire that synchronizes with
  // the Write release).
  SequenceNumber m_begin;       // The node where the release sequence begins.
  SequenceNumber m_end;         // The node where the release sequence ends.

  id_type m_id;
  boolean::Variable m_boolexpr_variable;
  static id_type s_next_release_sequence_id;    // The id to use for the next ReleaseSequence.
                                                // The first ReleaseSequence object has id 0.

  // Construct a new id / boolean variable pair.
  ReleaseSequence(SequenceNumber begin, SequenceNumber end) :
      m_begin(begin), m_end(end),
      m_id(s_next_release_sequence_id++),
      m_boolexpr_variable(boolean::Context::instance().create_variable(id_name()))
    { Dout(dc::notice, "Created a new ReleaseSequence(" << begin << ", " << end << "), id " << m_id << ", with m_boolexpr_variable " << m_boolexpr_variable); }

  id_type id() const { return m_id; }
  std::string id_name() const;
  boolean::Variable boolexpr_variable() const { return m_boolexpr_variable; }
  bool equals(SequenceNumber rs_begin, SequenceNumber rs_end) const { return m_begin == rs_begin && m_end == rs_end; }
  bool equals(ReleaseSequence const& release_sequence) const { return m_begin == release_sequence.m_begin && m_end == release_sequence.m_end; }

  friend std::ostream& operator<<(std::ostream& os, ReleaseSequence const& release_sequence);
};
