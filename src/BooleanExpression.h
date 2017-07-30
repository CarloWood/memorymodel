#pragma once

#include "utils/Singleton.h"
#include <iosfwd>
#include <map>

namespace boolean_expression {

class Context;
class Product;
class Expression;

class VariableData
{
 private:
  int m_user_id;
  std::string m_name;

 public:
  VariableData(int user_id, std::string const& name) : m_user_id(user_id), m_name(name) { }

  int user_id() const { return m_user_id; }
  std::string const& name() const { return m_name; }

  friend std::ostream& operator<<(std::ostream& os, VariableData const& variable_data);
};

class Variable
{
 public:
  using id_type = unsigned int;

 private:
  friend class Product;
  id_type m_id;
  static id_type s_next_id;

 public:
  Variable() { }
  Variable(id_type id) : m_id(id) {}
  Variable(Variable const& variable) : m_id(variable.m_id) {}
  Variable& operator=(Variable const& variable)
  {
    m_id = variable.m_id;
    return *this;
  }
  Product operator~() const;

 private:
  friend class Context;
  static Variable create_variable() { return s_next_id++; }

  friend bool operator<(Variable const& variable1, Variable const& variable2) { return variable1.m_id < variable2.m_id; }
  friend std::ostream& operator<<(std::ostream& os, Variable const& variable);
};

class Context : public Singleton<Context>
{
  friend_Instance;
 public:
  using VariableKey = Variable;
  using variables_type = std::map<VariableKey, VariableData>;  

 private:
  variables_type m_variables;

 private:
  Context() { }
  ~Context() { }
  Context(Context const&) = delete;

 public:
  Variable make_variable(int user_id, std::string const& name);
  VariableData const& operator()(Variable variable) const;
};

struct Product
{
 public:
  using mask_type = uint64_t;
  static size_t constexpr mask_size = sizeof(mask_type) * 8;    // Size of mask_type in bits.
  static mask_type constexpr empty_mask = 0;
  static mask_type constexpr full_mask = empty_mask - 1;
  static mask_type constexpr zero_mask = 1;                     // The variable bit representing zero.
 
 private:
  friend class Expression;
  mask_type m_variables;        // Variables in use have their bit set.
  mask_type m_inverted;         // Variables in use that are inverted.

  static mask_type to_mask(Variable variable) { ASSERT(variable.m_id < mask_size - 1); return mask_type{2} << variable.m_id; }

 public:
  Product() { }
  Product(bool literal) : m_variables(zero_mask), m_inverted(literal ? zero_mask : empty_mask) { }
  Product(Variable variable) : m_variables(to_mask(variable)), m_inverted(empty_mask) { }

  void invert() { m_inverted ^= full_mask; m_inverted &= m_variables; }
  bool is_sane() const;

  Product& operator*=(Product const& product)
  {
    // If either side (this or product) is 0 or 1 then we have a special case.
    // If product (and/or this) is a special case then product.m_variables == 0 and the first condition is always true.
    if (((m_inverted ^ product.m_inverted) & (m_variables & product.m_variables)) == 0)
    {
      m_inverted = (m_variables & m_inverted) | (product.m_variables & product.m_inverted);
      m_variables |= product.m_variables;
      // Is one side a literal (not both sides, because then the above already works)?
      if ((m_variables & zero_mask) && (m_variables & ~zero_mask))
      {
        // Is the literal a one?
        if ((m_inverted & zero_mask))
        {
          // Erase the one from the product.
          m_variables &= ~zero_mask;
          m_inverted &= ~zero_mask;
        }
        else
        {
          // The result is zero.
          m_variables = zero_mask;
          m_inverted = empty_mask;
        }
      }
    }
    else
    {
      // If we get here then this is NOT a special case.
      // We get here when we AND an X with a NOT X, which is always false.
      // Set the product to represent False:
      m_variables = zero_mask;
      m_inverted = empty_mask;
    }
    return *this;
  }

  Product operator*(Product const& rhs) const { Product result(*this); result *= rhs; return result; }
  Product operator~() const { Product result(*this); result.invert(); return result; }

  bool is_literal() const { return m_variables & zero_mask; }
  bool is_zero() const { return m_variables & ~m_inverted & zero_mask; }
  bool is_one() const { return m_variables & m_inverted & zero_mask; }

  friend std::ostream& operator<<(std::ostream& os, Product const& product);
  friend bool operator==(Product const& product1, Product const& product2)
      { return product1.m_variables == product2.m_variables && product1.m_inverted == product2.m_inverted; }
  friend bool operator!=(Product const& product1, Product const& product2)
      { return product1.m_variables != product2.m_variables || product1.m_inverted != product2.m_inverted; }
};

class Expression
{
 public:
  using mask_type = Product::mask_type;

 private:
  using sum_of_products_type = std::vector<Product>;
  sum_of_products_type m_sum_of_products;       // Elements must have a unique set of variables (Product::m_variables) and be ordered.

  static mask_type grayToBinary64(mask_type num)
  {
    num ^= num >> 32;
    num ^= num >> 16;
    num ^= num >> 8;
    num ^= num >> 4;
    num ^= num >> 2;
    num ^= num >> 1;
    return num;
  }

  // Used for ordering m_sum_of_products.
  static bool less(Product const& product1, Product const& product2)
  {
    // Note: we need to sort on inverted too for the sake of simplify(),
    // even though after simplifying that compare becomes irrelevant.
    // Moreover this ordering needs to according to Gray code for the
    // benefit of simplify().
    return product1.m_variables < product2.m_variables ||
           (product1.m_variables == product2.m_variables &&
            grayToBinary64(product1.m_inverted) < grayToBinary64(product2.m_inverted)); // mask_type must be unsigned for this to work.
  }

 public:
  Expression() { }
  Expression(Expression&& expression) : m_sum_of_products(std::move(expression.m_sum_of_products)) { }
  Expression& operator=(Expression&& expression) { m_sum_of_products = std::move(expression.m_sum_of_products); return *this; }
  explicit Expression(Product const& product) : m_sum_of_products(1, product) { }
  Expression(bool literal) : m_sum_of_products(1, Product(literal)) { }
  Expression copy() const { Expression result; result.m_sum_of_products = m_sum_of_products; return result; }
  static Expression zero() { Expression result(Product{false}); return result; }
  static Expression one() { Expression result(Product{true}); return result; }

  friend Expression operator+(Expression const& expression0, Expression const& expression1);
  Expression& operator+=(Expression const& expression) { *this = std::move(*this + expression); return *this; }
  Expression& operator+=(Product const& product);
  Expression operator*(Product const& product) const;
  void simplify();
  void sanity_check() const;

  // A literal (zero or one) can only be in a sum when it is the only term.
  bool is_literal() const { return m_sum_of_products[0].is_literal(); }
  bool is_zero() const { return m_sum_of_products[0].is_zero(); }
  bool is_one() const { return m_sum_of_products[0].is_one(); }
  bool equivalent(Expression const& expression) const;

  friend std::ostream& operator<<(std::ostream& os, Expression const& expression);
  friend bool operator==(Expression const& expression1, Expression const& expression2) { return expression1.m_sum_of_products == expression2.m_sum_of_products; }
  friend bool operator!=(Expression const& expression1, Expression const& expression2) { return !(expression1.m_sum_of_products == expression2.m_sum_of_products); }
};

} // namespace boolean_expression
