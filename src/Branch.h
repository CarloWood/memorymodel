#pragma once

#include "Condition.h"
#include <iosfwd>

struct Evaluation;

struct Branch
{
  conditions_type::iterator m_condition;
  bool m_condition_true;

  Condition::id_type id() const { return m_condition->id(); }
  boolexpr::bx_t boolean_expression() const { return m_condition_true ? m_condition->boolexpr_variable() : ~m_condition->boolexpr_variable(); }
  Branch(conditions_type::iterator const& condition, bool condition_true) : m_condition(condition), m_condition_true(condition_true) { }
  friend std::ostream& operator<<(std::ostream& os, Branch const& branch);
};
