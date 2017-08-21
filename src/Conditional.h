#pragma once

#include "Condition.h"
#include <string>

struct Evaluation;
struct Context;

struct Conditional
{
  using id_type = int;

  id_type m_id;
  boolean::Variable m_boolexpr_variable;
  static id_type s_next_conditional_id;               // The id to use for the next Conditional.

  // Construct a new id / boolean variable pair.
  Conditional() :
      m_id(s_next_conditional_id++),
      m_boolexpr_variable(boolean::Context::instance().create_variable(id_name()))
    { Dout(dc::notice, "Created a new Conditional with m_boolexpr_variable " << m_boolexpr_variable); }

  id_type id() const { return m_id; }
  std::string id_name() const;
  boolean::Variable boolexpr_variable() const { return m_boolexpr_variable; }
  friend std::ostream& operator<<(std::ostream& os, Conditional const& conditional);
};

using conditionals_type = std::map<Evaluation*, Conditional>;
