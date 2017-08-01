#include "sys.h"
#include "BooleanExpression.h"
#include "utils/is_power_of_two.h"
#include "utils/MultiLoop.h"
#include <ostream>
#include <bitset>

namespace boolean_expression {

std::ostream& operator<<(std::ostream& os, VariableData const& variable_data)
{
  os << '{' << variable_data.m_user_id << ", " << variable_data.m_name << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, Variable const& variable)
{
  os << Context::instance()(variable.m_id);
  return os;
}

std::string Product::to_string(bool html) const
{
  if (is_literal())
    return is_one() ? "1" : "0";

  std::string result;
  for (Variable::id_type id = 0; id < Product::max_number_of_variables; ++id)
  {
    mask_type variable = to_mask(id);
    if (!(m_variables & variable))      // Is this variable used?
    {
      bool inverted = m_negation & variable;
      for (char c : Context::instance()(id).name())
      {
        if (!html && inverted)
          result += "\u0305";
        result += c;
        if (html && inverted)
          result += "&#x305;";
      }
    }
  }
  return result;
}

std::ostream& operator<<(std::ostream& os, Expression const& expression)
{
  bool first = true;
  for (auto&& product : expression.m_sum_of_products)
  {
    if (!first)
      os << " + ";
    os << product;
    first = false;
  }
  return os;
}

std::string Expression::as_html_string() const
{
  std::string result;
  bool first = true;
  for (auto&& product : m_sum_of_products)
  {
    if (!first)
      result += '+';
    result += product.to_string(true);
    first = false;
  }
  return result;
}

Variable Context::create_variable(std::string const& name, int user_id)
{
  auto res = m_variables.emplace(Variable(), VariableData(name, user_id));
  return res.first->first;
}

VariableData const& Context::operator()(Variable::id_type id) const
{
  variables_type::const_iterator res = m_variables.find(id);
  // Don't call this for Variable's that weren't created with Context::create_variable.
  ASSERT(res != m_variables.end());
  return res->second;
}

Expression& Expression::operator+=(Product const& product)
{
  // FIXME - this can be optimized.
  Expression expr(product);
  *this += expr;
  return *this;
}

Expression Expression::operator*(Product const& product) const
{
  Expression result(0);
  for (auto&& term : m_sum_of_products)
    result += term * product;
  return result;
}

Expression operator+(Expression const& expression0, Expression const& expression1)
{
  size_t size[2] = { expression0.m_sum_of_products.size(), expression1.m_sum_of_products.size() };

  // An empty vector means the Expression is undefined!
  ASSERT(size[0] > 0 && size[1] > 0);

  // Merge the two ordered vectors.
  Expression merged;
  merged.m_sum_of_products.reserve(size[0] + size[1]);

  Expression::sum_of_products_type const* expressions[2] = { &expression0.m_sum_of_products, &expression1.m_sum_of_products };

  if (expression0.is_literal() || expression1.is_literal())
  {
    // If either side is a literal then simplify immediately.
    int ret = expression0.is_one() || expression1.is_zero() ? 0 : 1;
    for (auto&& product : *expressions[ret])
      merged.m_sum_of_products.push_back(product);
  }
  else
  {
    // Zip the two vectors into eachother so the result is still ordered.
    size_t index[2] = { 0, 0 };
    int largest;
    do
    {
      // Sort large to small (many variables to few), so that when simplify removes variables from a term we
      // get something that might still combine with a term that it still has to process.
      largest = Expression::less(expression0.m_sum_of_products[index[0]], expression1.m_sum_of_products[index[1]]) ? 1 : 0;
      merged.m_sum_of_products.push_back((*expressions[largest])[index[largest]++]);
    }
    while (index[largest] < size[largest]);
    int remaining = 1 - largest;
    do
    {
      merged.m_sum_of_products.push_back((*expressions[remaining])[index[remaining]++]);
    }
    while (index[remaining] < size[remaining]);
    //merged.simplify();
  }

  return merged;
}

void Expression::simplify()
{
  int size = m_sum_of_products.size();

  // An empty vector means the Expression is undefined!
  ASSERT(size > 0);

  if (size == 1)
    return;

  //--------------------------------------------------------------------------
  // Remove duplicated entries.

  int tail = 0, head = 1;
  // D
  // D                          <-- head
  // C
  // C           <-- head
  // C           <-- tail       <-- tail
  // B <-- head     ^              ^
  // A <-- tail      \__ point A.   \__ point B.

  // Find the first two entries that are equal.
  do
  {
    if (m_sum_of_products[tail] == m_sum_of_products[head])
      break;
  }
  while (++tail, ++head < size);
  // Did we find two equal terms?
  if (head < size)
  {
    // Point A, see above.
    // Find the first head that is not equal to tail.
    while (++head < size)
      if (m_sum_of_products[tail] != m_sum_of_products[head])
        break;
    if (head < size)
    {
      // Point B, see above.
      // Start writing from head to tail.
      do
      {
        m_sum_of_products[++tail] = m_sum_of_products[head];
        // Skip terms that we already have.
        while (++head < size && m_sum_of_products[tail] == m_sum_of_products[head]);
      }
      while (head < size);
    }
    // Truncate the vector to its final size.
    size = tail + 1;
    m_sum_of_products.resize(size);
  }

  //-----------------------------------------------------------------------------------------------------
  // Simplify PX + PX' = P, where P is any product (not containing X) and X is a single boolean variable.

  if (size > 1)
  {
    // Find two adjacent terms comprised of the same variables.
    //
    // Product terms that have the same variables are ordered
    // according to the Gray code encoding the NOT operator.
    // For example,
    //
    // 0  0  0  0  1  1  1  1   <-- variable A
    // 0  0  1  1  1  1  0  0   <-- variable B
    // 0  1  1  0  0  1  1  0   <-- variable C
    //
    // shows that the following sum of three products would be ordered as
    //
    // A              A'    A'
    // B  +           B' +  B
    // C              C'    C
    //
    // This already has the property that although no
    // two terms are adjacent that have only a single NOT
    // operator change, two such terms exist (the first
    // and the last).

    sum_of_products_type::iterator term_n = m_sum_of_products.begin();
    sum_of_products_type::iterator term_n_plus_one = term_n + 1;
    while (term_n_plus_one != m_sum_of_products.end())
    {
      if (term_n->m_variables == term_n_plus_one->m_variables)
      {
        mask_type changed_NOT_operators = term_n->m_negation ^ term_n_plus_one->m_negation;
        if (utils::is_power_of_two(changed_NOT_operators))     // Only a single NOT operator changed?
        {
          Product remaining;
          remaining.m_variables = term_n->m_variables | changed_NOT_operators;
          remaining.m_negation = term_n->m_negation | changed_NOT_operators;
          if (remaining.m_variables == Product::full_mask)
          {
            // Set outselves to one.
            m_sum_of_products.resize(1);
            m_sum_of_products[0] = true;
            return;
          }
          term_n = m_sum_of_products.erase(term_n, term_n_plus_one + 1);
          if (term_n == m_sum_of_products.end())
          {
            m_sum_of_products.push_back(remaining);
            break;
          }
          int skipped = 0;
          // When for example inserting remaining == x̅y in the list
          //            .-- end
          //            v
          //  x̅z xz x y
          //  ^
          //  |
          // term_n
          //
          // then this will find first_smaller_term pointing to x:
          sum_of_products_type::iterator first_smaller_term(term_n);
          while (first_smaller_term != m_sum_of_products.end() &&
                 less(remaining, *first_smaller_term))    // Is remaining still less than first_smaller_term?
          {                                               // advance to an even simpler term.
            ++first_smaller_term;
            ++skipped;
          }
          // Then insert remaining before first_smaller_term (reassign term_n because the insert might invalidate it),
          // provided they aren't the same of course!
          if (first_smaller_term == m_sum_of_products.end() ||
              *first_smaller_term != remaining)
          {
            term_n = m_sum_of_products.insert(first_smaller_term, remaining) - skipped;
            term_n_plus_one = term_n + 1;
          }
          continue;
        }
      }
      ++term_n;
      ++term_n_plus_one;
    }
  }

    // ZD  + ZD'Y = Z (D + D'Y) = ZDY' + ZY -->
    // ZD  + ZD'Y + Z'Y + ZY' + Z'Y' = ZDY' + (ZY + Z'Y + ZY' + Z'Y') = 1
}

bool Product::is_sane() const
{
  if (m_variables == empty_mask && m_negation == full_mask)
  {
    ASSERT(is_literal() && is_zero());
    return true;
  }
  if (m_variables == full_mask && m_negation == empty_mask)
  {
    ASSERT(is_literal() && is_one());
    return true;
  }
  ASSERT(!is_literal());
  mask_type not_used = ~all_variables;
  // Unused variables have their bit set.
  ASSERT((m_variables & not_used) == not_used);
  // Also in m_negation.
  ASSERT((m_negation & not_used) == not_used);
  ASSERT((m_negation & m_variables) == m_variables);
  return true;
}

void Expression::sanity_check() const
{
  ASSERT(!m_sum_of_products.empty());
  ASSERT(m_sum_of_products[0].is_sane());
  ASSERT(!m_sum_of_products[0].is_literal() || m_sum_of_products.size() == 1);
  for (auto&& term : m_sum_of_products)
    ASSERT(term.is_sane());
  for (auto iter = m_sum_of_products.begin(); iter != m_sum_of_products.end(); ++iter)
  {
    auto next = iter + 1;
    if (next == m_sum_of_products.end())
      break;
    ASSERT(*next != *iter);
    ASSERT(less(*next, *iter));
  }
}

// Brute force comparison of two boolean expressions.
bool Expression::equivalent(Expression const& expression) const
{
  mask_type all_variables = 0;
  for (auto&& product : m_sum_of_products)
    if (!product.is_literal())
      all_variables |= ~product.m_variables;
  for (auto&& product : expression.m_sum_of_products)
    if (!product.is_literal())
      all_variables |= ~product.m_variables;
  int number_of_variables = 0;
  int variable_id[Product::max_number_of_variables];
  for (Variable::id_type id = 0; id < Product::max_number_of_variables; ++id)
  {
    mask_type bit = Product::to_mask(id);
    if ((all_variables & bit))
      variable_id[number_of_variables++] = id;
  }
  // Now we can find back the bit(mask) of a variable with: Product::to_mask(variable_id[variable]).
  int number_of_permutations = 1 << number_of_variables;
  for (int permutation = 0; permutation < number_of_permutations; ++permutation)
  {
    mask_type set_variables = 0;
    for (int variable = 0; variable < number_of_variables; ++variable)
    {
      int permbit = 1 << variable;
      if ((permutation & permbit))
        set_variables |= Product::to_mask(variable_id[variable]);
    }
    bool result1 = is_one();
    if (!is_literal())
    {
      result1 = false;
      for (auto&& product : m_sum_of_products)
        if ((~product.m_variables & (set_variables ^ product.m_negation)) == ~product.m_variables)
        {
          result1 = true;
          break;
        }
    }
    bool result2 = expression.is_one();
    if (!expression.is_literal())
    {
      result2 = false;
      for (auto&& product : expression.m_sum_of_products)
        if ((~product.m_variables & (set_variables ^ product.m_negation)) == ~product.m_variables)
        {
          result2 = true;
          break;
        }
    }
    if (result1 != result2)
      return false;
  }
  return true;
}

//static
Variable::id_type Variable::s_next_id;

// The Context singleton.
static SingletonInstance<Context> dummy __attribute__ ((__unused__));

} // namespace boolean_expression
