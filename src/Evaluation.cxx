#include "sys.h"
#include "debug.h"
#include "Evaluation.h"
#include "Context.h"
#include "Graph.h"
#include "TagCompare.h"
#include "utils/macros.h"
#include "utils/AIAlert.h"
#include <iostream>

char const* code(binary_operators op)
{
  switch (op)
  {
    case multiplicative_mo_mul:
      return "*";
    case multiplicative_mo_div:
      return "/";
    case multiplicative_mo_mod:
      return "%";
    case additive_ado_add:
      return "+";
    case additive_ado_sub:
      return "-";
    case shift_so_shl:
      return "<<";
    case shift_so_shr:
      return ">>";
    case relational_ro_lt:
      return "<";
    case relational_ro_gt:
      return ">";
    case relational_ro_ge:
      return ">=";
    case relational_ro_le:
      return "<=";
    case equality_eo_eq:
      return "==";
    case equality_eo_ne:
      return "!=";
    case bitwise_and:
      return "&";
    case bitwise_exclusive_or:
      return "^";
    case bitwise_inclusive_or:
      return "|";
    case logical_and:
      return "&&";
    case logical_or:
      return "||";
  }
  return "UNKNOWN binary_operators";
}

char const* operator_str(binary_operators op)
{
  switch (op)
  {
    AI_CASE_RETURN(multiplicative_mo_mul);
    AI_CASE_RETURN(multiplicative_mo_div);
    AI_CASE_RETURN(multiplicative_mo_mod);
    AI_CASE_RETURN(additive_ado_add);
    AI_CASE_RETURN(additive_ado_sub);
    AI_CASE_RETURN(shift_so_shl);
    AI_CASE_RETURN(shift_so_shr);
    AI_CASE_RETURN(relational_ro_lt);
    AI_CASE_RETURN(relational_ro_gt);
    AI_CASE_RETURN(relational_ro_ge);
    AI_CASE_RETURN(relational_ro_le);
    AI_CASE_RETURN(equality_eo_eq);
    AI_CASE_RETURN(equality_eo_ne);
    AI_CASE_RETURN(bitwise_and);
    AI_CASE_RETURN(bitwise_exclusive_or);
    AI_CASE_RETURN(bitwise_inclusive_or);
    AI_CASE_RETURN(logical_and);
    AI_CASE_RETURN(logical_or);
  }
  return "UNKNOWN binary_operators";
}

std::ostream& operator<<(std::ostream& os, binary_operators op)
{
  return os << operator_str(op);
}

char const* state_str(Evaluation::State state)
{
  switch (state)
  {
    AI_CASE_RETURN(Evaluation::unused);
    AI_CASE_RETURN(Evaluation::uninitialized);
    AI_CASE_RETURN(Evaluation::literal);
    AI_CASE_RETURN(Evaluation::variable);
    AI_CASE_RETURN(Evaluation::pre);
    AI_CASE_RETURN(Evaluation::post);
    AI_CASE_RETURN(Evaluation::unary);
    AI_CASE_RETURN(Evaluation::binary);
    AI_CASE_RETURN(Evaluation::condition);
  }
  return "UNKNOWN Evaluation::State";
}

std::ostream& operator<<(std::ostream& os, Evaluation::State state)
{
  return os << state_str(state);
}

char const* increment_str(int increment)
{ 
  if (increment > 0)
    return "++";
  return "--";
}

void Evaluation::print_on(std::ostream& os) const
{
  switch (m_state)
  {
    case Evaluation::unused:
      os << "<\e[31mUNUSED Evaluation\e[0m>";
      break;
    case Evaluation::uninitialized:
      os << "<UNINITIALIZED Evaluation>";
      break;
    case Evaluation::literal:
      os << m_simple.m_literal;
      break;
    case Evaluation::variable:
      os << m_simple.m_variable;
      break;
    case Evaluation::pre:
      os << increment_str(m_simple.m_increment) << *m_lhs;
      break;
    case Evaluation::post:
      os << *m_lhs << increment_str(m_simple.m_increment);
      break;
    case Evaluation::unary:
      os << code(m_operator.unary) << '(' << *m_lhs << ')';
      break;
    case Evaluation::binary:
      os << '(' << *m_lhs << ") " << code(m_operator.binary) << " (" << *m_rhs << ')';
      break;
    case Evaluation::condition:
      os << '(' << *m_condition << ") ? (" << *m_lhs << ") : (" << *m_rhs << ')';
      break;
  }
  bool first = true;
  for (auto&& node : m_value_computations)
  {
    os << (first ? '/' : ',') << *node;
    first = false;
  }
  first = true;
  for (auto&& node : m_side_effects)
  {
    os << (first ? '/' : ',') << *node;
    first = false;
  }
}

std::ostream& operator<<(std::ostream& os, Evaluation const& value_computation)
{
  value_computation.print_on(os << '{');
  return os << '}';
}

//static
void Evaluation::negate(std::unique_ptr<Evaluation>& ptr)
{
  DoutEntering(dc::simplify, "Evaluation::negate(" << *ptr << ").");
  if (ptr->m_state == literal)
  {
    ptr->m_simple.m_literal = -ptr->m_simple.m_literal;
    Dout(dc::simplify, "It was a literal; now it is: " << *ptr);
    return;
  }
  else if (ptr->is_negated())
  {
    ptr = std::move(ptr->m_lhs);
    Dout(dc::simplify, "It was negated; now it is: " << *ptr);
  }
  else
  {
    Evaluation negated;
    negated.m_state = unary;
    negated.m_operator.unary = ast::uo_minus;
    negated.m_lhs = std::move(ptr);
    ptr = make_unique(std::move(negated));
    Dout(dc::simplify, "Now it is: " << *ptr);
  }
}

void Evaluation::swap_sum()
{
  // Do not call this function unless the following holds:
  ASSERT(is_sum() && m_lhs->m_state == literal && m_rhs->m_state != literal);
  Dout(dc::simplify, "Swapping " << *m_lhs << " with " << *m_rhs << '.');

  // This should never happen:
  // The lhs of the rhs should not be a literal (if the rhs is a sum).
  ASSERT(!m_rhs->is_sum() || m_rhs->m_lhs->m_state != literal);
  // The rhs should not be a negated value computation if it is subtracted.
  ASSERT(m_operator.binary != additive_ado_sub || !m_rhs->is_negated());

  // (1) - (...)
  // becomes
  // -(...) + (1)
  std::swap(m_lhs, m_rhs);
  if (m_operator.binary == additive_ado_sub)
  {
    // Negate right-hand side.
    m_rhs->m_simple.m_literal = -m_rhs->m_simple.m_literal;
    // Negate left-hand side.
    if (m_lhs->is_negated())
    {
      m_lhs = std::move(m_lhs->m_lhs);
    }
    else if (m_lhs->is_sum() &&
        (m_lhs->m_lhs->m_state == literal ||
         m_lhs->m_lhs->is_negated() ||
         m_lhs->m_rhs->m_state == literal ||
         m_lhs->m_rhs->is_negated()))
    {
      Dout(dc::simplify, "Before negating both sides: " << *this);
      // Can't call unary_operator because that assumes a Evaluation on the stack :/
      negate(m_lhs->m_lhs);     //->unary_operator(ast::uo_minus);
      if (m_lhs->m_rhs->is_negated())
        negate(m_lhs->m_rhs);   // ->unary_operator(ast::uo_minus);
      else if (m_lhs->m_operator.binary == additive_ado_sub)
        m_lhs->m_operator.binary = additive_ado_add;
      else if (m_lhs->m_rhs->m_state == literal)
        m_lhs->m_rhs->m_simple.m_literal = -m_lhs->m_rhs->m_simple.m_literal;
      else
        m_lhs->m_operator.binary = additive_ado_sub;
      Dout(dc::simplify, "After negating both sides: " << *this);
    }
    else
    {
      Evaluation* unary_value_computation = new Evaluation;
      unary_value_computation->m_allocated = true;
      unary_value_computation->m_lhs = std::move(m_lhs);
      m_lhs = std::move(std::unique_ptr<Evaluation>(unary_value_computation));
      m_lhs->m_state = unary;
      m_lhs->m_operator.unary = ast::uo_minus;
    }
  }
}

void Evaluation::strip_rhs()
{
  m_rhs.reset();
  m_state = m_lhs->m_state;
  m_simple = m_lhs->m_simple;
  m_operator = m_lhs->m_operator;
  m_rhs = std::move(m_lhs->m_rhs);
  m_condition = std::move(m_lhs->m_condition);
  m_value_computations = std::move(m_lhs->m_value_computations);
  m_side_effects = std::move(m_lhs->m_side_effects);
  m_lhs = std::move(m_lhs->m_lhs);
}

void Evaluation::OP(binary_operators op, Evaluation&& rhs)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::OP(" << op << ", " << rhs << ") [this = " << *this << "] ==> ");
  // Should never try to use an unused or uninitialized Evaluation in a binary operator.
  ASSERT(m_state != unused);
  if (m_state == uninitialized)
    THROW_ALERT("Applying binary operator `[OPERATOR]` to uninitialized variable `[VARIABLE]`", AIArgs("[OPERATOR]", op)("[VARIABLE]", *this));
  if (m_state == literal && rhs.m_state == literal)
  {
    switch (op)
    {
      case multiplicative_mo_mul:
        m_simple.m_literal *= rhs.m_simple.m_literal;
        break;
      case multiplicative_mo_div:
        m_simple.m_literal /= rhs.m_simple.m_literal;
        break;
      case multiplicative_mo_mod:
        m_simple.m_literal %= rhs.m_simple.m_literal;
        break;
      case additive_ado_add:
        m_simple.m_literal += rhs.m_simple.m_literal;
        break;
      case additive_ado_sub:
        m_simple.m_literal -= rhs.m_simple.m_literal;
        break;
      case shift_so_shl:
        m_simple.m_literal <<= rhs.m_simple.m_literal;
        break;
      case shift_so_shr:
        m_simple.m_literal >>= rhs.m_simple.m_literal;
        break;
      case relational_ro_lt:
        m_simple.m_literal = m_simple.m_literal < rhs.m_simple.m_literal;
        break;
      case relational_ro_gt:
        m_simple.m_literal = m_simple.m_literal > rhs.m_simple.m_literal;
        break;
      case relational_ro_ge:
        m_simple.m_literal = m_simple.m_literal >= rhs.m_simple.m_literal;
        break;
      case relational_ro_le:
        m_simple.m_literal = m_simple.m_literal <= rhs.m_simple.m_literal;
        break;
      case equality_eo_eq:
        m_simple.m_literal = m_simple.m_literal == rhs.m_simple.m_literal;
        break;
      case equality_eo_ne:
        m_simple.m_literal = m_simple.m_literal != rhs.m_simple.m_literal;
        break;
      case bitwise_and:
        m_simple.m_literal &= rhs.m_simple.m_literal;
        break;
      case bitwise_exclusive_or:
        m_simple.m_literal ^= rhs.m_simple.m_literal;
        break;
      case bitwise_inclusive_or:
        m_simple.m_literal |= rhs.m_simple.m_literal;
        break;
      case logical_and:
        m_simple.m_literal = m_simple.m_literal && rhs.m_simple.m_literal;
        break;
      case logical_or:
        m_simple.m_literal = m_simple.m_literal || rhs.m_simple.m_literal;
        break;
    }
  }
  else
  {
    m_lhs = make_unique(std::move(*this));
    m_rhs = make_unique(std::move(rhs));
    m_operator.binary = op;
    m_state = binary;

    // Simplify sums.
    if (op == additive_ado_add || op == additive_ado_sub)
    {
      Dout(dc::simplify, "Simplifying " << *this << '.');

      // Trying to subtract a negated value computation?
      if (op == additive_ado_sub && m_rhs->is_negated())
      {
        m_rhs = std::move(m_rhs->m_lhs);
        m_operator.binary = additive_ado_add;
        Dout(dc::simplify, "1. " << *this);
      }

      //      m_operator.binary (== op)
      //         |
      //         v
      // (A + B) - (C + D)
      // ^         ^
      // |         |
      // m_lhs     m_rhs

      // If the lhs is a literal, swap it with the rhs.
      if (m_lhs->m_state == literal)
      {
        swap_sum();     // This might change m_operator.binary invalidating op!
        Dout(dc::simplify, "2. " << *this);
      }
      if (m_rhs->m_state == literal)
      {
        // (A + B) - (3) --> (A + B) + (-3)
        if (m_operator.binary == additive_ado_sub)
        {
          m_operator.binary = additive_ado_add;
          m_rhs->m_simple.m_literal = -m_rhs->m_simple.m_literal;
          Dout(dc::simplify, "3. " << *this);
        }
        // (5 + (A)) + (-3) --> ((A) + 5) + (-3) --> (A) + 2
        if (m_lhs->is_sum())
        {
          if (m_lhs->m_lhs->m_state == literal)
          {
            m_lhs->swap_sum();     // This might change m_operator.binary invalidating op!
            Dout(dc::simplify, "4. " << *this);
          }
          if (m_lhs->m_rhs->m_state == literal)
          {
            ASSERT(m_lhs->m_operator.binary == additive_ado_add && m_operator.binary == additive_ado_add);
            m_lhs->m_rhs->m_simple.m_literal += m_rhs->m_simple.m_literal;
            strip_rhs();
            Dout(dc::simplify, "5. " << *this);
          }
        }
      }
      else if (m_lhs->is_sum() && m_lhs->m_rhs->m_state == literal)
      {
        if (m_rhs->is_sum() && m_rhs->m_rhs->m_state == literal)
        {
          m_rhs->m_rhs->m_simple.m_literal += m_lhs->m_rhs->m_simple.m_literal;
          m_lhs->strip_rhs();
          Dout(dc::simplify, "6. " << *this);
        }
        else
        {
          std::swap(m_lhs->m_operator.binary, m_operator.binary);
          std::swap(m_lhs->m_rhs, m_rhs);       // (A + 3) - (...) --> (A - (...)) + 3
          Dout(dc::simplify, "7. " << *this);
        }
      }
      if (is_sum() && m_rhs->m_state == literal && m_rhs->m_simple.m_literal == 0)
      {
        strip_rhs();
        Dout(dc::simplify, "8. " << *this);
      }
    }
  }
  Dout(dc::finish, *this << '.');
}

void Evaluation::postfix_operator(ast::postfix_operators op)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::postfix_operator(" << op << ") [this = " << *this << "] ==> ");
  // Should never try to apply a postfix operator to an unused or uninitialized Evaluation.
  ASSERT(m_state != unused);
  if (m_state == uninitialized)
    THROW_ALERT("Applying postfix operator `[OPERATOR]` to uninitialized variable `[VARIABLE]`", AIArgs("[OPERATOR]", op)("[VARIABLE]", *this));
  m_lhs = make_unique(std::move(*this));
  m_state = post;
  m_simple.m_increment = op == ast::po_inc ? 1 : -1;
  Dout(dc::finish, *this << '.');
}

void Evaluation::prefix_operator(ast::unary_operators op)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::prefix_operator(" << op << ") [this = " << *this << "] ==> ");
  // Should never try to apply a prefix operator to an unused or uninitialized Evaluation.
  ASSERT(m_state != unused);
  if (m_state == uninitialized)
    THROW_ALERT("Applying prefix operator `[OPERATOR]` to uninitialized variable `[VARIABLE]`", AIArgs("[OPERATOR]", op)("[VARIABLE]", *this));
  // Call unary_operator for these values.
  ASSERT(op == ast::uo_inc || op == ast::uo_dec);
  m_lhs = make_unique(std::move(*this));
  m_state = pre;
  m_simple.m_increment = op == ast::uo_inc ? 1 : -1;
  Dout(dc::finish, *this << '.');
}

void Evaluation::read(ast::tag tag, Context& context)
{
  context.read(tag, *this);
}

void Evaluation::read(ast::tag tag, std::memory_order mo, Context& context)
{
  context.read(tag, *this);
}

void Evaluation::write(ast::tag tag, Context& context)
{
  context.write(tag, std::move(*this));
}

void Evaluation::write(ast::tag tag, std::memory_order mo, Context& context)
{
  context.write(tag, mo, std::move(*this));
}

void Evaluation::add_value_computation(std::set<Node>::iterator const& node)
{
  DoutEntering(dc::notice, "Evaluation::add_value_computation(" << *node << ") [this is " << *this << "].");
  // FIXME: I think that we always only have at most a single value computation?
  ASSERT(m_value_computations.empty());
  m_value_computations.push_back(node);
}

void Evaluation::add_side_effect(std::set<Node>::iterator const& node)
{
  DoutEntering(dc::notice, "Evaluation::add_side_effect(" << *node << ") [this is " << *this << "].");
  // FIXME: I think that we always only have at most a single side effect?
  ASSERT(m_value_computations.empty());
  m_side_effects.push_back(node);
}

Evaluation::Evaluation(Evaluation&& value_computation) :
    m_state(value_computation.m_state),
    m_allocated(value_computation.m_allocated),
    m_simple(value_computation.m_simple),
    m_operator(value_computation.m_operator),
    m_lhs(std::move(value_computation.m_lhs)),
    m_rhs(std::move(value_computation.m_rhs)),
    m_condition(std::move(value_computation.m_condition)),
    m_value_computations(std::move(value_computation.m_value_computations)),
    m_side_effects(std::move(value_computation.m_side_effects))
{
  ASSERT(m_state != unused);
  // Make sure we won't use this again!
  value_computation.m_state = unused;
}

void Evaluation::operator=(Evaluation&& value_computation)
{
  m_state = value_computation.m_state;
  m_allocated = value_computation.m_allocated;
  m_simple = value_computation.m_simple;
  m_operator = value_computation.m_operator;
  m_lhs = std::move(value_computation.m_lhs);
  m_rhs = std::move(value_computation.m_rhs);
  m_condition = std::move(value_computation.m_condition);
  m_value_computations = std::move(value_computation.m_value_computations);
  m_side_effects = std::move(value_computation.m_side_effects);
  // Make sure we won't use this again!
  value_computation.m_state = unused;
}

//static
std::unique_ptr<Evaluation> Evaluation::make_unique(Evaluation&& value_computation)
{
  assert(!value_computation.m_allocated);
  Evaluation* new_value_computation = new Evaluation(std::move(value_computation));
  new_value_computation->m_allocated = true;
  return std::unique_ptr<Evaluation>(new_value_computation);
}

void Evaluation::unary_operator(ast::unary_operators op)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::unary_operator(" << op << ") [this = " << *this << "] ==> ");
  // Should never try to apply a unary operator to an unused or uninitialized Evaluation.
  ASSERT(m_state != unused);
  if (m_state == uninitialized)
    THROW_ALERT("Applying unary operator `[OPERATOR]` to uninitialized variable `[VARIABLE]`", AIArgs("[OPERATOR]", op)("[VARIABLE]", *this));
  // Call prefix_operator for these values.
  ASSERT(op != ast::uo_inc && op != ast::uo_dec);
  if (m_state == literal)
  {
    switch (op)
    {
      case ast::uo_plus:
        break;
      case ast::uo_minus:
        m_simple.m_literal = -m_simple.m_literal;
        break;
      case ast::uo_not:
        m_simple.m_literal = !m_simple.m_literal;
        break;
      case ast::uo_invert:
        m_simple.m_literal = ~m_simple.m_literal;
        break;
      default:
        THROW_ALERT("Cannot apply unary operator `[OPERATOR]` to literal value ([LITERAL])", AIArgs("[OPERATOR]", code(op))("[LITERAL]", m_simple.m_literal));
    }
  }
  else if (op == ast::uo_minus && is_negated())
    *this = std::move(*m_lhs);
  else
  {
    m_lhs = make_unique(std::move(*this));
    m_operator.unary = op;
    m_state = unary;
  }
  Dout(dc::finish, *this << '.');
}

void Evaluation::conditional_operator(Evaluation&& true_value, Evaluation&& false_value)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::conditional_operator(" << true_value << ", " << false_value << ") [this = " << *this << "] ==> ");
  if (m_state == literal)
  {
    Dout(dc::simplify, "Simplifying because condition is a literal...");
    if (m_simple.m_literal)
      *this = std::move(true_value);
    else
      *this = std::move(false_value);
  }
  else
  {
    m_condition = make_unique(std::move(*this));
    m_state = condition;
    m_lhs = make_unique(std::move(true_value));
    m_rhs = make_unique(std::move(false_value));
  }
  Dout(dc::finish, *this << '.');
}

void Evaluation::print_tree(Context& context, bool recursive) const
{
  static std::set<ast::tag, TagCompare> dirty;
  if (!recursive)
    dirty.clear();
  if (m_state == variable && !dirty.insert(m_simple.m_variable).second)
  {
    Dout(dc::evaltree, *this << " (see above).");
    return;
  }

  Dout(dc::evaltree, *this);
  debug::Mark mark;

  switch (m_state)
  {
    case unused:
    case uninitialized:
    case literal:
      // We're done.
      break;
    case variable:
    {
      ast::tag tag = m_simple.m_variable;
      context.m_graph.for_each_write_node(tag,
          [](Node const& node, Context& context)
          {
            Evaluation const* evaluation = node.get_evaluation();
            evaluation->print_tree(context, true);
          }, context);
      break;
    }
    case pre:
      m_lhs->print_tree(context, true);
      break;
    case post:
      m_lhs->print_tree(context, true);
      break;
    case unary:
      m_lhs->print_tree(context, true);
      break;
    case binary:
      m_lhs->print_tree(context, true);
      m_rhs->print_tree(context, true);
      break;
    case condition:
      m_condition->print_tree(context, true);
      m_lhs->print_tree(context, true);
      m_rhs->print_tree(context, true);
      break;
  }
}

void Evaluation::for_each_node(std::function<void(Node const&)> const& action) const
{
  DoutEntering(dc::notice, "Evaluation::for_each_node(...) [this = " << *this << "].");
  ASSERT(m_state == variable || (m_value_computations.empty() && m_side_effects.empty()));
  Dout(dc::notice, m_state);
  switch (m_state)
  {
    case unused:
      assert(m_state != unused);
      break;
    case uninitialized:
      break;
    case literal:
      break;
    case variable:
    {
      Dout(dc::notice, "Size of m_value_computations = " << m_value_computations.size());
      for(auto&& node : m_value_computations)
        action(*node);
      Dout(dc::notice, "Size of m_side_effects = " << m_side_effects.size());
      for(auto&& node : m_side_effects)
        action(*node);
      break;
    }
    case pre:
      m_lhs->for_each_node(action);
      break;
    case post:
      m_lhs->for_each_node(action);
      break;
    case unary:
      m_lhs->for_each_node(action);
      break;
    case binary:
      m_lhs->for_each_node(action);
      m_rhs->for_each_node(action);
      break;
    case condition:
      m_condition->for_each_node(action);
      m_lhs->for_each_node(action);
      m_rhs->for_each_node(action);
      break;
  }
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct valuecomp("VALUECOMP");
channel_ct simplify("SIMPLIFY");
channel_ct evaltree("EVALTREE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
