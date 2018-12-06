#pragma once

#include "ast_tag.h"
#include "ast_operators.h"
#include "debug.h"
#include "EvaluationNodePtrs.h"
#include "NodePtr.h"
#include "EdgeType.h"
#include <iosfwd>
#include <vector>
#include <set>
#include <functional>

#ifdef TRACK_EVALUATION
#include "tracked.h"
#endif

class NodeRequestedType;

enum binary_operators {
  multiplicative_mo_mul,
  multiplicative_mo_div,
  multiplicative_mo_mod,
  additive_ado_add,
  additive_ado_sub,
  shift_so_shl,
  shift_so_shr,
  relational_ro_lt,
  relational_ro_gt,
  relational_ro_ge,
  relational_ro_le,
  equality_eo_eq,
  equality_eo_ne,
  bitwise_and,
  bitwise_exclusive_or,
  bitwise_inclusive_or,
  logical_and,
  logical_or
};

std::ostream& operator<<(std::ostream& os, binary_operators op);

#ifdef TRACK_EVALUATION
extern char const* name_Evaluation;
#endif

class Evaluation
#ifdef TRACK_EVALUATION
    : public tracked::Tracked<&name_Evaluation>
#endif
{
#ifdef TRACK_EVALUATION
  using tracked::Tracked<&name_Evaluation>::Tracked;
#endif

 public:
  enum Unused { not_used };
  enum State { unused, uninitialized, literal, variable, pre, post, unary, binary, condition, comma };  // See also is_valid.

 private:
  State m_state;
  bool m_allocated;
  union Simple
  {
    int m_literal;                              // Only valid when m_state == literal.
    int m_increment;                            // Only valid when m_state == pre or post; either -1 or +1.
    ast::tag m_variable;                        // Only valid when m_state == variable.
    Simple() { }
    explicit Simple(int literal) : m_literal(literal) { }
    explicit Simple(ast::tag tag) : m_variable(tag) { }
  } m_simple;
  union
  {
    ast::unary_operators unary;                 // Only valid when m_state == unary.
    binary_operators binary;                    // Only valid when m_state == binary.
  } m_operator;
  std::unique_ptr<Evaluation> m_lhs;            // Only valid when m_state == pre, post, unary, binary or condition.
  std::unique_ptr<Evaluation> m_rhs;            // Only valid when m_state == binary or condition.
  std::unique_ptr<Evaluation> m_condition;      // Only valid when m_state == condition  (m_condition ? m_lhs : m_rhs).
  std::vector<NodePtr> m_value_computations;    // And RMW nodes, because we never look for nodes that are JUST side-effect.
  std::vector<NodePtr> m_side_effects;          // Nodes that are not value_computations.

 public:
  Evaluation() : m_state(uninitialized), m_allocated(false) { }
  Evaluation(Evaluation&& value_computation);
  explicit Evaluation(Unused) : m_state(unused), m_allocated(false) { }
  Evaluation(int value) : m_state(literal), m_allocated(false), m_simple(value) { }
  Evaluation(ast::tag tag) : m_state(variable), m_allocated(false), m_simple(tag) { }
  Evaluation& operator=(int value)
  {
#ifdef TRACK_EVALUATION
    ASSERT(entry()->status_below(Tracked::Entry::pillaged));
#endif
    m_state = literal;
    m_simple.m_literal = value;
    return *this;
  }
  Evaluation& operator=(ast::tag tag)
  {
#ifdef TRACK_EVALUATION
    ASSERT(entry()->status_below(Tracked::Entry::pillaged));
#endif
    m_state = variable;
    m_simple.m_variable = tag;
    return *this;
  }
  void operator=(Evaluation&& value_computation);

  // Shallow-copy value_computation and turn it into an allocation.
  static std::unique_ptr<Evaluation> make_unique(Evaluation&& value_computation);
  // Apply negation unary operator - to the Evaluation object pointed to by ptr.
  static void negate(std::unique_ptr<Evaluation>& ptr);

  bool is_valid() const
  {
#ifdef TRACK_EVALUATION
    if (!entry()->status_below(Tracked::Entry::pillaged))
      DoutFatal(dc::core, "Calling is_valid() on a uninitialized Evaluation (" << *this << ").");
#endif
    return m_state > uninitialized;
  }
  bool is_sum() const { return m_state == binary && (m_operator.binary == additive_ado_add || m_operator.binary == additive_ado_sub); }
  bool is_negated() const { return m_state == unary && m_operator.unary == ast::uo_minus; }
  bool is_allocated() const { return m_allocated; }
  bool is_literal() const { return m_state == literal; }
  int literal_value() const { ASSERT(m_state == literal); return m_simple.m_literal; }

  void OP(binary_operators op, Evaluation&& rhs);         // *this OP= rhs.
  void postfix_operator(ast::postfix_operators op);       // (*this)++ or (*this)--
  void prefix_operator(ast::unary_operators op);          // ++*this or --*this
  void unary_operator(ast::unary_operators op);           // *this = OP *this
  void conditional_operator(EvaluationNodePtrs const& before_node_ptrs,      // *this = *this ? true_evaluation : false_evaluation
                            Evaluation&& true_evaluation,
                            Evaluation&& false_evaluation);
  void comma_operator(Evaluation&& rhs);
  void read(ast::tag tag);
  void read(ast::tag tag, std::memory_order mo);
  void add_value_computation(NodePtr const& node);
  void write(ast::tag tag, bool side_effect_sb_value_computation = false);
  NodePtr write(ast::tag tag, std::memory_order mo);
  NodePtr RMW(ast::tag tag, std::memory_order mo);
  NodePtr compare_exchange_weak(ast::tag tag, ast::tag expected, int desired, std::memory_order success, std::memory_order fail);
  void add_side_effect(NodePtr const& node);
  void destruct(DEBUG_ONLY(EdgeType edge_type));
  void swap_sum();
  void strip_rhs();

  friend std::ostream& operator<<(std::ostream& os, Evaluation const& value_computation);
  friend char const* state_str(State state);

  void print_on(std::ostream& os) const;
  void for_each_node(NodeRequestedType const& requested_type, std::function<void(NodePtr const&)> const& action COMMA_DEBUG_ONLY(EdgeType edge_type)) const;
  EvaluationNodePtrs get_nodes(NodeRequestedType const& requested_type COMMA_DEBUG_ONLY(EdgeType edge_type)) const;

  // Accessors used to print RMW node labels. See RMWNode::print_code.
  State state() const { return m_state; }
  binary_operators binary_operator() const { ASSERT(m_state == binary); return m_operator.binary; }
  Evaluation const* rhs() const { ASSERT(m_state == binary); return m_rhs.get(); }
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct valuecomp;
extern channel_ct simplify;
extern channel_ct evaltree;
NAMESPACE_DEBUG_CHANNELS_END
#endif
