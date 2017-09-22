#pragma once

#include "boolean-expression/BooleanExpression.h"
#include <iosfwd>

class NodeProvidedType
{
 public:
  enum Type {
    value_computation,
    value_computation_and_side_effect = value_computation,
    side_effect
  };

 private:
  Type m_type;

 public:
  NodeProvidedType(Type provided_type) : m_type(provided_type) { }
  Type type() const { return m_type; }

  friend std::ostream& operator<<(std::ostream& os, NodeProvidedType const& provided_type);
};

class NodeRequestedType
{
 public:
  enum Type {
    value_computation_heads_,
    heads_,
    tails_,
    all_
  };

 public:
 static NodeRequestedType const value_computation_heads;
 static NodeRequestedType const heads;
 static NodeRequestedType const tails;
 static NodeRequestedType const all;

 private:
  Type m_type;

 public:
  NodeRequestedType(Type requested_type) : m_type(requested_type) { }

  bool any() const { return m_type == all_; }
  bool matches(NodeProvidedType const& provided_type) const;
  Type type() const { return m_type; }

  friend std::ostream& operator<<(std::ostream& os, NodeRequestedType const& requested_type);
};

class SBNodePresence
{
 private:
  boolean::Expression m_sequenced_before_value_computation;  // We are sequenced before a value-computation Node.
  boolean::Expression m_sequenced_before_side_effect;        // We are sequenced before a side-effect Node.
  bool m_sequenced_after_something;                          // Hence, not a tail.
  bool m_sequenced_before_pseudo_value_computation;          // We pretend to be sequenced before a value-computation Node.

 public:
  SBNodePresence() :
      m_sequenced_before_value_computation(0),
      m_sequenced_before_side_effect(0),
      m_sequenced_after_something(false),
      m_sequenced_before_pseudo_value_computation(false) { }

  bool sequenced_before_pseudo_value_computation() const { return m_sequenced_before_pseudo_value_computation; }
  void set_sequenced_before_pseudo_value_computation() { m_sequenced_before_pseudo_value_computation = true; }
  boolean::Expression hiding_behind_another(NodeRequestedType const& requested_type) const;
  boolean::Expression const& provides_sequenced_before_value_computation() const { return m_sequenced_before_value_computation; }
  boolean::Expression const& provides_sequenced_before_side_effect() const { return m_sequenced_before_side_effect; }
  bool provides_sequenced_after_something() const { return m_sequenced_after_something; }
  bool update_sequenced_before_value_computation(bool node_provides, boolean::Expression&& sequenced_before_value_computation);
  bool update_sequenced_before_side_effect(bool node_provides, boolean::Expression&& sequenced_before_side_effect);
  void set_sequenced_after_something();

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, SBNodePresence const& sb_node_presence) { sb_node_presence.print_on(os); return os; }
};
