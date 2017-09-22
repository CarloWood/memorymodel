#pragma once

#include "boolean-expression/BooleanExpression.h"
#include <iosfwd>

struct Branch;

// The condition under which an edge exists,
// which might or might not depend on a conditional.
//
// If the condition is 1 then it is not depending on a conditional.
//
class Condition
{
 private:
  boolean::Product m_boolean_product;   // Either one (1) (not a branch)
                                        // or a single indeterminate boolean variable (one branch of the conditional that it represents),
                                        // or its negation thereof (the other branch of the conditional that it represents).

 public:
  Condition() : m_boolean_product(true) { }
  Condition(Branch const&);

  void reset() { m_boolean_product = true; }
  bool conditional() const { return !m_boolean_product.is_one(); }
  boolean::Product const& boolean_product() const { return m_boolean_product; }

  void operator*=(Condition const& condition) { m_boolean_product *= condition.m_boolean_product; }

  friend bool operator!=(Condition const& condition1, Condition const& condition2) { return condition1.m_boolean_product != condition2.m_boolean_product; }
  friend std::ostream& operator<<(std::ostream& os, Condition const& condition);
};
