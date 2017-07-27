#pragma once

#include "boolexpr/boolexpr.h"
#include <iosfwd>

class NodeProvidedType
{
 public:
  enum Type {
    value_computation,
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
  static int const sequenced_before_value_computation_        = 0;       // We are sequenced before a value-computation Node.
  static int const sequenced_before_side_effect_              = 1;       // We are sequenced before a side-effect Node.
  static int const array_size                                 = 2;

 private:
  boolexpr::array_t m_bxs;
  bool m_sequenced_after_something;             // Hence, not a tail.
  bool m_sequenced_before_pseudo_value_computation;
  static boolexpr::zero_t s_zero;
  static char const* const s_name[array_size];

 public:
  SBNodePresence() :
      m_bxs{new boolexpr::Array({s_zero, s_zero})},
      m_sequenced_after_something(false),
      m_sequenced_before_pseudo_value_computation(false) { }

  bool sequenced_before_pseudo_value_computation() const { return m_sequenced_before_pseudo_value_computation; }
  void set_sequenced_before_pseudo_value_computation() { m_sequenced_before_pseudo_value_computation = true; }
  boolexpr::bx_t hiding_behind_another(NodeRequestedType const& requested_type);
  boolexpr::bx_t provides_sequenced_before_value_computation() const { return (*m_bxs)[sequenced_before_value_computation_]; }
  boolexpr::bx_t provides_sequenced_before_side_effect() const { return (*m_bxs)[sequenced_before_side_effect_]; }
  bool provides_sequenced_after_something() const { return m_sequenced_after_something; }
  bool update_sequenced_before_value_computation(bool node_provides, boolexpr::bx_t sequenced_before_value_computation);
  bool update_sequenced_before_side_effect(bool node_provides, boolexpr::bx_t sequenced_before_side_effect);
  void set_sequenced_after_something();

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, SBNodePresence const& sb_node_presence) { sb_node_presence.print_on(os); return os; }
};
