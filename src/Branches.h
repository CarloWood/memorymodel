#pragma once

#include "Branch.h"
#include "BooleanExpression.h"
#include <iosfwd>

class Branches
{
 private:
  boolean::Product m_boolean_product;

 public:
  Branches() : m_boolean_product(1) { }
  Branches(Branches const&) = delete;
  Branches(Branches&& branches) : m_boolean_product(std::move(branches.m_boolean_product)) {  }
  Branches(Branch const&);
  Branches& operator=(Branches const&) = delete;
  void operator=(Branches&&) = delete;

  void operator&=(Branches const& branch);
  bool conditional() const { return !m_boolean_product.is_one(); }
  boolean::Product const& boolean_product() const { return m_boolean_product; }

  friend std::ostream& operator<<(std::ostream& os, Branches const& branches);
};
