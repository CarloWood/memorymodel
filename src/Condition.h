#pragma once

#include "BooleanExpression.h"
#include <set>
#include <string>
#include <iosfwd>

struct Evaluation;
struct Context;

struct Condition
{
  using id_type = int;

  id_type m_id;
  boolean_expression::Variable m_boolexpr_variable;
  static id_type s_next_condition_id;               // The id to use for the next Condition.

  // Construct a new id / boolean variable pair.
  Condition() :
      m_id(s_next_condition_id++),
      m_boolexpr_variable(boolean_expression::Context::instance().create_variable(id_name()))
    { Dout(dc::notice, "Created a new Condition with m_boolexpr_variable " << m_boolexpr_variable); }

  id_type id() const { return m_id; }
  std::string id_name() const;
  boolean_expression::Product boolexpr_variable() const { return m_boolexpr_variable; }
  friend std::ostream& operator<<(std::ostream& os, Condition const& condition);
};

using conditions_type = std::map<Evaluation*, Condition>;
