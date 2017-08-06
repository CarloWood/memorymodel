#pragma once

#include "Branch.h"
#include "BooleanExpression.h"
#include <iosfwd>

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
  Condition() : m_boolean_product(1) { }
  Condition(Condition const& condition) : m_boolean_product(condition.m_boolean_product) {  }
  Condition(Branch const&);

  void reset() { m_boolean_product = 1; }
  bool conditional() const { return !m_boolean_product.is_one(); }
  boolean::Product const& boolean_product() const { return m_boolean_product; }

  friend std::ostream& operator<<(std::ostream& os, Condition const& condition);
};
