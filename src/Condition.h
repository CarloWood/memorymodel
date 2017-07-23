#pragma once

#include "boolexpr/boolexpr.h"
#include <set>
#include <string>
#include <iosfwd>

struct Evaluation;
struct Context;

struct Condition
{
  using id_type = int;

  id_type m_id;
  Evaluation* m_condition;
  mutable boolexpr::var_t m_boolexpr_variable;

  Condition(int& next_id, Evaluation* condition) : m_id(next_id++), m_condition(condition) { }
  id_type id() const { return m_id; }
  std::string id_name() const;
  Evaluation const& evaluation() const { return *m_condition; }
  void create_boolexpr_variable(Context& context) const;
  boolexpr::var_t boolexpr_variable() const { return m_boolexpr_variable; }
  friend bool operator<(Condition const& cond1, Condition const& cond2) { return cond1.m_condition < cond2.m_condition; }
  friend std::ostream& operator<<(std::ostream& os, Condition const& condition);
};

using conditions_type = std::set<Condition>;
