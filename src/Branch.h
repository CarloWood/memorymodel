#pragma once

#include "Conditional.h"
#include <iosfwd>

struct Evaluation;

// A Branch is a wrapper around a pointer (iterator) to a (unique) Conditional object
// with an additional member variable that indicates if the condition is true or false.
//
// if (Conditional)
//   Branch(conditional, true)
// else
//   Branch(conditional, false)

struct Branch
{
  conditionals_type::iterator m_conditional;  // The Conditional that this branch depends on.
  bool m_conditional_true;                         // Whether the Conditional must be true or false.

  Conditional::id_type id() const { return m_conditional->second.id(); }
  boolean::Product boolean_product() const { return boolean::Product(m_conditional->second.boolexpr_variable(), !m_conditional_true); }
  Branch(conditionals_type::iterator const& conditional, bool conditional_true) :
      m_conditional(conditional), m_conditional_true(conditional_true) { }
  friend std::ostream& operator<<(std::ostream& os, Branch const& branch);
};
