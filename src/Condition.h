#pragma once

#include "BooleanExpression.h"
#include <set>
#include <string>
#include <iosfwd>

struct Evaluation;
struct Context;
enum Foo { foo };
struct Condition
{
  using id_type = int;

  id_type m_id;
  Evaluation* m_condition;
  boolean_expression::Variable m_boolexpr_variable;
  static id_type s_next_condition_id;               // The id to use for the next Condition.

  // Construct a Condition with just the key set.
  Condition(Evaluation* condition, Foo) : m_condition(condition), m_boolexpr_variable(133) { }

  // Construct a new Condition for condition.
  Condition(Evaluation* condition) :
      m_id(s_next_condition_id++),
      m_condition(condition),
      m_boolexpr_variable(boolean_expression::Context::instance().create_variable(id_name()))
    { Dout(dc::notice, "Created a new Condition for condtion " << (void*)condition << "; m_boolexpr_variable is now " << m_boolexpr_variable.id()); }

  id_type id() const { return m_id; }
  std::string id_name() const;
  Evaluation const& evaluation() const { return *m_condition; }
  boolean_expression::Product boolexpr_variable() const { return m_boolexpr_variable; }
  friend bool operator<(Condition const& cond1, Condition const& cond2) { return cond1.m_condition < cond2.m_condition; }
  friend std::ostream& operator<<(std::ostream& os, Condition const& condition);
};

using conditions_type = std::set<Condition>;
