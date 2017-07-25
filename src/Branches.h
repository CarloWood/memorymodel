#pragma once

#include "Branch.h"
#include "boolexpr/boolexpr.h"
#include <iosfwd>

class Branches
{
 private:
  boolexpr::bx_t m_boolean_expression;
  static boolexpr::one_t s_one;

 public:
  Branches() : m_boolean_expression(s_one) { }
  Branches(Branches const&) = delete;
  Branches(Branches&& branches) : m_boolean_expression(std::move(branches.m_boolean_expression)) {  }
  Branches(Branch const&);
  Branches& operator=(Branches const&) = delete;
  void operator=(Branches&&) = delete;

  void operator&=(Branches const& branch);
  bool conditional() const { return !m_boolean_expression->equiv(s_one); }
  boolexpr::bx_t boolean_expression() const { return m_boolean_expression; }

  friend std::ostream& operator<<(std::ostream& os, Branches const& branches);
};
