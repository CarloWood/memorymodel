#include "sys.h"
#include "debug.h"
#include "Evaluation.h"
#include "Context.h"
#include "Graph.h"
#include "TagCompare.h"
#include "iomanip_html.h"
#include "utils/macros.h"
#include "utils/AIAlert.h"
#include <ostream>

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
    AI_CASE_RETURN(Evaluation::comma);
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

enum Colors { reset, red, lightgray };

char const* color_code[3][2] = {
  { "\e[0m", "</FONT>" },
  { "\e[31m", "<FONT COLOR=\"red\">" },
  { "\e[37m", "<FONT COLOR=\"gray\">" }
};

std::string color(Colors color, bool html)
{
  return color_code[color][html];
}

std::string to_html(char const* str)
{
  std::string result;
  for (char const* p = str; *p; ++p)
  {
    switch (*p)
    {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      default:
        result += *p;
    }
  }
  return result;
}

void Evaluation::print_on(std::ostream& os) const
{
#ifdef TRACK_EVALUATION
  DoutEntering(dc::notice, "Evaluation::print_on(" << (void*)&os << ") [this = " << (void*)this << " (" << static_cast<Tracked const&>(*this) << ")]");
#endif

  bool const html = IOManipHtml::is_html(os);
  static int suppress_color_index = std::ios_base::xalloc();
  bool print_color_codes = !os.iword(suppress_color_index);

  os << '{';
  switch (m_state)
  {
    case Evaluation::unused:
      if (print_color_codes)
        os << (html ? "&lt;" : "<") << color(red, html) << "UNUSED Evaluation" << color(reset, html) << (html ? "&gt;" : ">");
      else
        os << "<UNUSED Evaluation>";
      break;
    case Evaluation::uninitialized:
      os << "<uninitialized>";
      break;
    case Evaluation::literal:
      os << m_simple.m_literal;
      break;
    case Evaluation::variable:
      os << m_simple.m_variable;
      break;
    case Evaluation::pre:
      os << increment_str(m_simple.m_increment);
      m_lhs->print_on(os);
      break;
    case Evaluation::post:
      m_lhs->print_on(os);
      os << increment_str(m_simple.m_increment);
      break;
    case Evaluation::unary:
    {
      char const* unary_op = code(m_operator.unary);
      if (html)
        os << to_html(unary_op) << '(';
      else
        os << unary_op << '(';
      m_lhs->print_on(os);
      os << ')';
      break;
    }
    case Evaluation::binary:
    {
      os << '(';
      m_lhs->print_on(os);
      char const* binary_op = code(m_operator.binary);
      if (html)
        os << ") " << to_html(binary_op) << " (";
      else
        os << ") " << binary_op << " (";
      m_rhs->print_on(os);
      os << ')';
      break;
    }
    case Evaluation::condition:
      os << '(';
      m_condition->print_on(os);
      os << ") ? (";
      m_lhs->print_on(os);
      os << ") : (";
      m_rhs->print_on(os);
      os << ')';
      break;
    case Evaluation::comma:
      os << '(';
      m_lhs->print_on(os);
      os << ", ";
      m_rhs->print_on(os);
      os << ')';
      break;
  }
  bool first = true;
  bool orig_print_color_codes = print_color_codes;
  for (auto&& node : m_value_computations)
  {
    if (!print_color_codes)
      os << (first ? "/" : ",");
    else
    {
      os << (first ? color(lightgray, html) + "/" : ",");
      if (first)
      {
        print_color_codes = false;
        os.iword(suppress_color_index) = 1;
      }
    }
    os << *node;
    first = false;
  }
  if (orig_print_color_codes && !print_color_codes)
  {
    os << color(reset, html);
    print_color_codes = true;
    os.iword(suppress_color_index) = 0;
  }
  first = true;
  for (auto&& node : m_side_effects)
  {
    if (!print_color_codes)
      os << (first ? "/" : ",");
    else
    {
      os << (first ? color(lightgray, html) + "/" : ",");
      if (first)
      {
        print_color_codes = false;
        os.iword(suppress_color_index) = 1;
      }
    }
    os << *node;
    first = false;
  }
  if (orig_print_color_codes && !print_color_codes)
  {
    os << color(reset, html);
    print_color_codes = true;
    os.iword(suppress_color_index) = 0;
  }
#ifdef TRACK_EVALUATION
  Debug(
    if (dc::tracked.is_on())
    {
      os << '(' << static_cast<Tracked const&>(*this) << ')';
    }
  );
#endif
  os << '}';
}

std::ostream& operator<<(std::ostream& os, Evaluation const& value_computation)
{
  value_computation.print_on(os);
  return os;
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
      m_lhs = std::unique_ptr<Evaluation>(unary_value_computation);
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
#ifdef TRACK_EVALUATION
    refresh();
#endif
    m_state = binary;
    m_operator.binary = op;
    m_rhs = make_unique(std::move(rhs));

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
#ifdef TRACK_EVALUATION
  refresh();
#endif
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
#ifdef TRACK_EVALUATION
  refresh();
#endif
  m_state = pre;
  m_simple.m_increment = op == ast::uo_inc ? 1 : -1;
  Dout(dc::finish, *this << '.');
}

void Evaluation::read(ast::tag tag)
{
  Context::instance().read(tag, *this);
}

void Evaluation::read(ast::tag tag, std::memory_order mo)
{
  Context::instance().read(tag, mo, *this);
}

void Evaluation::write(ast::tag tag, bool side_effect_sb_value_computation)
{
  Context::instance().write(tag, std::move(*this), side_effect_sb_value_computation);
}

NodePtr Evaluation::write(ast::tag tag, std::memory_order mo)
{
  return Context::instance().write(tag, mo, std::move(*this));
}

NodePtr Evaluation::RMW(ast::tag tag, std::memory_order mo)
{
  return Context::instance().RMW(tag, mo, std::move(*this));
}

NodePtr Evaluation::compare_exchange_weak(
    ast::tag tag, ast::tag expected, int desired, std::memory_order success, std::memory_order fail)
{
  return Context::instance().compare_exchange_weak(tag, expected, desired, success, fail, std::move(*this));
}

void Evaluation::add_value_computation(NodePtr const& node)
{
  DoutEntering(dc::sb_edge, "Evaluation::add_value_computation(" << *node << ") [this is " << *this << "].");
  // FIXME: I think that we always only have at most a single value computation?
  ASSERT(m_value_computations.empty());
  m_value_computations.push_back(node);
}

void Evaluation::add_side_effect(NodePtr const& node)
{
  DoutEntering(dc::sb_edge, "Evaluation::add_side_effect(" << *node << ") [this is " << *this << "].");
  // FIXME: I think that we always only have at most a single side effect?
  ASSERT(m_value_computations.empty() || node->is_second_mutex_access());
  m_side_effects.push_back(node);
}

//   Foo(Foo& orig) : tracked::Tracked<&name_Foo>{orig}, ... { }
//   Foo(Foo const& orig) : tracked::Tracked<&name_Foo>{orig}, ... { }
//   Foo(Foo&& orig) : tracked::Tracked<&name_Foo>{std::move(orig)}, ... { }
//   Foo(Foo const&& orig) : tracked::Tracked<&name_Foo>{std::move(orig)}, ... { }
//   void operator=(Foo const& orig) { tracked::Tracked<&name_Foo>::operator=(orig); ... }
//   void operator=(Foo&& orig) { tracked::Tracked<&name_Foo>::operator=(std::move(orig)); ... }

Evaluation::Evaluation(Evaluation&& value_computation) :
#ifdef TRACK_EVALUATION
    tracked::Tracked<&name_Evaluation>{std::move(value_computation)},
#endif
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
#ifdef TRACK_EVALUATION
  tracked::Tracked<&name_Evaluation>::operator=(std::move(value_computation));
#endif
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
  {
    m_state = m_lhs->m_state;
    // Make sure we won't use this again!
    m_lhs->m_state = unused;
    m_allocated = false;
    m_simple = m_lhs->m_simple;
    m_operator = m_lhs->m_operator;
    m_rhs = std::move(m_lhs->m_rhs);
    m_condition = std::move(m_lhs->m_condition);
    m_value_computations = std::move(m_lhs->m_value_computations);
    m_side_effects = std::move(m_lhs->m_side_effects);
    m_lhs = std::move(m_lhs->m_lhs);
  }
  else
  {
    m_lhs = make_unique(std::move(*this));
#ifdef TRACK_EVALUATION
    refresh();
#endif
    m_state = unary;
    m_operator.unary = op;
  }
  Dout(dc::finish, *this << '.');
}

void Evaluation::destruct(DEBUG_ONLY(EdgeType edge_type))
{
  std::vector<NodePtr> nodes;
  for_each_node(NodeRequestedType::all,
      [&nodes](NodePtr const& node)
      {
        nodes.push_back(node);
      }
      COMMA_DEBUG_ONLY(edge_type)
  );
  for (NodePtr node : nodes)
    Context::instance().graph().remove_node(node);
}

void Evaluation::conditional_operator(
    EvaluationNodePtrs const& before_node_ptrs,
    Evaluation&& true_evaluation,
    Evaluation&& false_evaluation)
{
  EvaluationNodePtrs true_nodes = true_evaluation.get_nodes(NodeRequestedType::tails COMMA_DEBUG_ONLY(edge_sb));
  EvaluationNodePtrs false_nodes = false_evaluation.get_nodes(NodeRequestedType::tails COMMA_DEBUG_ONLY(edge_sb));
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::conditional_operator(" << true_evaluation << ", " << false_evaluation << ") [this = " << *this << "] ==> ");
  if (m_state == literal)
  {
    Dout(dc::simplify, "Simplifying because condition is a literal...");
    if (m_simple.m_literal)
    {
      *this = std::move(true_evaluation);
      false_evaluation.destruct(DEBUG_ONLY(edge_sb));
      Context::instance().add_edges(edge_sb, before_node_ptrs, true_nodes);
    }
    else
    {
      *this = std::move(false_evaluation);
      true_evaluation.destruct(DEBUG_ONLY(edge_sb));
      Context::instance().add_edges(edge_sb, before_node_ptrs, false_nodes);
    }
  }
  else
  {
    m_condition = make_unique(std::move(*this));
#ifdef TRACK_EVALUATION
    refresh();
#endif
    m_state = condition;
    m_lhs = make_unique(std::move(true_evaluation));
    m_rhs = make_unique(std::move(false_evaluation));
    auto condition = Context::instance().add_condition(m_condition);
    Context::instance().add_edges(edge_sb, before_node_ptrs, true_nodes, condition(true));
    Context::instance().add_edges(edge_sb, before_node_ptrs, false_nodes, condition(false));
  }
  Dout(dc::finish, *this << '.');
}

void Evaluation::comma_operator(Evaluation&& rhs)
{
  DoutEntering(dc::valuecomp|continued_cf, "Evaluation::comma_operator(" << rhs << ") [this = " << *this << "] == > ");
  m_lhs = make_unique(std::move(*this));
#ifdef TRACK_EVALUATION
    refresh();
#endif
  m_state = comma;
  m_rhs = make_unique(std::move(rhs));
  Dout(dc::finish, *this << '.');
}

#ifdef TRACK_EVALUATION
char const* name_Evaluation = "Evaluation";
#endif

void Evaluation::for_each_node(
    NodeRequestedType const& requested_type,
    std::function<void(NodePtr const&)> const& action
    COMMA_DEBUG_ONLY(EdgeType edge_type)) const
{
#ifdef CWDEBUG
  libcwd::channel_ct& debug_channel{*DEBUGCHANNELS::dc::edge[edge_type]};
#endif
  DoutEntering(debug_channel, "Evaluation::for_each_node(" << requested_type << ", ...) [this = " << *this << "].");
  ASSERT(m_state == variable || (m_value_computations.empty() && m_side_effects.empty()));
  Dout(debug_channel, m_state);
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
      Dout(debug_channel, "Size of m_value_computations = " << m_value_computations.size());
      for (auto&& node : m_value_computations)
      {
        boolean::Expression hiding;      // FIXME (!) hiding is not used?!
        if (node->matches(requested_type, hiding))
        {
          Dout(debug_channel, "Calling action(" << *node << ")");
          action(node);
        }
        else
        {
          Dout(debug_channel, "Call to action(" << *node << ") was skipped.");
        }
        if (WriteNode const* write_node = node.get<WriteNode>())   // RMW's are stored in m_value_computations and are writes!
          write_node->get_evaluation()->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      }
      Dout(debug_channel, "Size of m_side_effects = " << m_side_effects.size());
      for (auto&& node : m_side_effects)
      {
        boolean::Expression hiding;      // FIXME (!) hiding is not used?!
        if (node->matches(requested_type, hiding))
        {
          Dout(debug_channel, "Calling action(" << *node << ")");
          action(node);
        }
        else
        {
          Dout(debug_channel, "Call to action(" << *node << ") was skipped.");
        }
        if (WriteNode const* write_node = node.get<WriteNode>())
          write_node->get_evaluation()->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      }
      break;
    }
    case pre:
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
    case post:
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
    case unary:
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
    case binary:
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      m_rhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
    case condition:
      m_condition->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      m_rhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
    case comma:
      m_lhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      m_rhs->for_each_node(requested_type, action COMMA_DEBUG_ONLY(edge_type));
      break;
  }
}

EvaluationNodePtrs Evaluation::get_nodes(NodeRequestedType const& requested_type COMMA_DEBUG_ONLY(EdgeType edge_type)) const
{
  DoutEntering(*dc::edge[edge_type], "Evaluation::get_nodes(" << requested_type << ") [this = " << *this << "]");
  EvaluationNodePtrs result;
  for_each_node(requested_type,
      [&result](NodePtr const& node_ptr)
      {
        result.push_back(node_ptr);
      }
  COMMA_DEBUG_ONLY(edge_type));
  Dout(*dc::edge[edge_type], "Returning EvaluationNodePtrs: " << result);
  return result;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct valuecomp("VALUECOMP");
channel_ct simplify("SIMPLIFY");
channel_ct evaltree("EVALTREE");
NAMESPACE_DEBUG_CHANNELS_END
#endif
