#pragma once

#include "Condition.h"
#include <iosfwd>

struct Evaluation;

struct Branch
{
  conditions_type::iterator m_condition;
  bool m_condition_true;

  Condition::id_type id() const { return m_condition->second.id(); }
  boolean_expression::Product boolean_product() const { return boolean_expression::Product(m_condition->second.boolexpr_variable(), !m_condition_true); }
  Branch(conditions_type::iterator const& condition, bool condition_true) : m_condition(condition), m_condition_true(condition_true) { }
  friend std::ostream& operator<<(std::ostream& os, Branch const& branch);
};
