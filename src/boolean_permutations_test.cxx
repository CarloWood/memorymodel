#include "sys.h"
#include "debug.h"
#include "BooleanExpression.h"
#include "utils/MultiLoop.h"
#include <iostream>

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  using namespace boolean;

  int const number_of_variables = 3;
  std::vector<Variable> v;
  char const* const names[] = { "w", "x", "y", "z" };
  for (int i = 0; i < number_of_variables; ++i)
    v.emplace_back(Context::instance().create_variable(names[i], i * i));

  std::vector<Product> products;
  //products.push_back(0);
  //products.push_back(1);

  for (int n = 1; n <= number_of_variables; ++n)
  {
    for (MultiLoop variable(n); !variable.finished(); variable.next_loop())
      for(; variable() < number_of_variables; ++variable)
      {
        if (*variable > 0 && variable() <= variable[*variable - 1])
        {
          variable.breaks(0);
          break;
        }
        if (variable.inner_loop())
        {
          for (MultiLoop inverted(n); !inverted.finished(); inverted.next_loop())
            for(; inverted() < 2; ++inverted)
            {
              if (inverted.inner_loop())
              {
                Product product(v[variable[0]], inverted[0]);
                for (int i = 1; i < n; ++i)
                {
                  Product next_factor(v[variable[i]], inverted[i]);
                  product *= next_factor;
                }
                products.push_back(product);
              }
            }
        }
      }
  }

  Dout(dc::notice, "Products:");
  for (auto&& p : products)
    Dout(dc::notice, p);

  int const number_of_products = products.size();

  std::vector<Expression> expressions;

  Dout(dc::notice, "Expressions:");
  for (int n = 1; n <= number_of_products; ++n)
  {
    for (MultiLoop term(n); !term.finished(); term.next_loop())
      for(; term() < number_of_products; ++term)
      {
        if (*term > 0 && term() <= term[*term - 1])
        {
          term.breaks(0);
          break;
        }
        if (term.inner_loop())
        {
          Expression original(products[term[0]]);
          expressions.emplace_back(products[term[0]]);
          for (int i = 1; i < n; ++i)
          {
            expressions.back() += products[term[i]];
            original += products[term[i]];
          }
          expressions.back().simplify();
          if (original != expressions.back())
          {
            Dout(dc::notice, original << " == " << expressions.back());
            ASSERT(expressions.back().equivalent(original));
          }
        }
      }
  }
}
